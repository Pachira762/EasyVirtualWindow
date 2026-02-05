package com.example.facetracker

import android.content.pm.PackageManager
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.opengl.GLES20
import android.opengl.GLSurfaceView
import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.example.facetracker.ui.theme.FaceTrackerTheme
import com.google.ar.core.AugmentedFace
import com.google.ar.core.CameraConfig
import com.google.ar.core.CameraConfigFilter
import com.google.ar.core.Config
import com.google.ar.core.Config.AugmentedFaceMode
import com.google.ar.core.Session
import com.google.ar.core.TrackingState
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

private const val DEVICE_ID: Int = 1

private const val TAG:String = "FaceTracker"

class MainActivity : ComponentActivity(), GLSurfaceView.Renderer, SensorEventListener {

    private lateinit var surfaceView: GLSurfaceView

    private var backgroundRenderer: BackgroundRenderer = BackgroundRenderer()

    private lateinit var session: Session

    private lateinit var sensorManager: SensorManager

    private lateinit var gravitySensor: Sensor

    private var oscClient: OscClient = OscClient("192.168.1.255", 11125)


    private var eyePosition  by mutableStateOf(FloatArray(3))

    private var gravityVector by mutableStateOf(FloatArray(3))

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        try {
            val cameraPermission = ContextCompat.checkSelfPermission(this, android.Manifest.permission.CAMERA)
            if (cameraPermission != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, arrayOf(android.Manifest.permission.CAMERA), 0)
                return
            }

            session = Session(this, setOf<Session.Feature>())

            val cameraConfigFilter = CameraConfigFilter(session).apply {
                setFacingDirection(CameraConfig.FacingDirection.FRONT)
            }
            val cameraConfigs = session.getSupportedCameraConfigs(cameraConfigFilter)
            session.cameraConfig = cameraConfigs[0]

            val config = Config(session).apply{
                setAugmentedFaceMode(AugmentedFaceMode.MESH3D)
            }
            session.configure(config)
        }
        catch(e: Exception) {
            Log.e(TAG, e.toString())
        }

        sensorManager = getSystemService(SENSOR_SERVICE) as SensorManager

        gravitySensor = sensorManager.getDefaultSensor(Sensor.TYPE_GRAVITY)!!

        surfaceView = GLSurfaceView(this).apply {
            preserveEGLContextOnPause = true
            setEGLContextClientVersion(2)
            setRenderer(this@MainActivity)
            renderMode = GLSurfaceView.RENDERMODE_CONTINUOUSLY
            setWillNotDraw(false)
        }

        setContent {
            FaceTrackerTheme {
                Scaffold(Modifier.fillMaxSize(), content = { innerPadding ->
                    @Suppress("COMPOSE_APPLIER_CALL_MISMATCH")
                    AndroidView(factory = { surfaceView }, modifier = Modifier.padding(innerPadding))
                    Column(
                        modifier = Modifier.padding(innerPadding),
                        content = {
                            Text("X = ${eyePosition[0]}\nY = ${eyePosition[1]}\nZ = ${eyePosition[2]}", fontSize = 32.sp, color = Color.White)
                            Text("Gx = ${gravityVector[0]}\nGy = ${gravityVector[1]}\nGz = ${gravityVector[2]}", fontSize = 32.sp, color = Color.White)
                        }
                    )
                })
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        session.close()
    }

    override fun onResume() {
        super.onResume()

        sensorManager.registerListener(this, gravitySensor, SensorManager.SENSOR_DELAY_NORMAL, SensorManager.SENSOR_DELAY_UI)
        session.resume()
        surfaceView.onResume()
    }

    override fun onPause() {
        super.onPause()

        sensorManager.unregisterListener(this, gravitySensor)
        surfaceView.onPause()
        session.pause()
    }

    override fun onSurfaceCreated(
        gl: GL10?,
        config: EGLConfig?
    ) {
        GLES20.glClearColor(0.1f, 0.1f, 0.1f, 1.0f)
        backgroundRenderer.onCreateInGLThread()
    }

    override fun onSurfaceChanged(
        gl: GL10?,
        width: Int,
        height: Int
    ) {
        GLES20.glViewport(0, 0, width, height)
        session.setDisplayGeometry(0, width, height)
    }

    override fun onDrawFrame(gl: GL10?) {
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT or GLES20.GL_DEPTH_BUFFER_BIT)

        val cameraTextureId = backgroundRenderer.cameraTextureId()
        session.setCameraTextureName(cameraTextureId)

        val frame = session.update()

        val face = session.getAllTrackables(AugmentedFace::class.java).firstOrNull()
        if (face != null && face.trackingState == TrackingState.TRACKING) {
            val pose = face.getCenterPose()
            val position = pose.transformPoint(floatArrayOf(0.0f, 0.04f, 0.02f)).let {
                floatArrayOf(-100.0f * it[2], 100.0f * it[0], 100.0f * it[1])
            }

            oscClient.send("/face-tracker/$DEVICE_ID/eye-position", position)

            runOnUiThread {
                eyePosition = position
            }
        }else{
            oscClient.send("/face-tracker/$DEVICE_ID/lost")
        }

        backgroundRenderer.draw(frame)
    }

    override fun onAccuracyChanged(sensor: Sensor, accuracy: Int){
    }

    override fun onSensorChanged(event: SensorEvent) {
        val gravity = event.values.let {
            floatArrayOf(100.0f * it[2], -100.0f * it[0], 100.0f * it[1])
        }

        oscClient.send("/face-tracker/$DEVICE_ID/gravity", gravity)

        runOnUiThread {
            System.arraycopy(gravity, 0, gravityVector, 0, gravityVector.size)
        }
    }
}
