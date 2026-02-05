/*
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This file is a Kotlin translation of the original Java source:
 * https://github.com/google-ar/arcore-android-sdk/tree/main/samples/augmented_faces_java/app/src/main/java/com/google/ar/core/examples/java/common/rendering/BackgroundRenderer.java
 *
 * Modifications and Kotlin translation:
 * Copyright 2026 Pachira762
 */

package com.example.facetracker

import android.opengl.GLES11Ext
import android.opengl.GLES20
import android.util.Log
import com.google.ar.core.Coordinates2d
import com.google.ar.core.Frame
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.FloatBuffer

private const val TAG:String = "FaceTracker.BackgroundRenderer"

class BackgroundRenderer {
    private val vertexShaderCode: String = """
attribute vec4 a_Position;
attribute vec2 a_TexCoord;

varying vec2 v_TexCoord;

void main() {
   gl_Position = a_Position;
   v_TexCoord = a_TexCoord;
} 
    """

    private val fragmentShaderCode: String = """
#extension GL_OES_EGL_image_external : require

precision mediump float;
varying vec2 v_TexCoord;
uniform samplerExternalOES sTexture;


void main() {
     gl_FragColor = texture2D(sTexture, v_TexCoord);
//    gl_FragColor = vec4(v_TexCoord, 0.0, 1.0);
}        
    """

    private val quadCoords: FloatBuffer = ByteBuffer.allocateDirect(4 * 8).let {
        it.order(ByteOrder.nativeOrder())
        it.asFloatBuffer()
    }.apply {
        put(floatArrayOf(-1.0f, -1.0f, +1.0f, -1.0f, -1.0f, +1.0f, +1.0f, +1.0f))
        position(0)
    }

    private val quadTexCoords: FloatBuffer = ByteBuffer.allocateDirect(4 * 8).let {
        it.order(ByteOrder.nativeOrder())
        it.asFloatBuffer()
    }

    private var cameraProgram: Int = -1

    private var cameraPositionAttrib: Int = -1

    private var cameraTexCoordAttrib: Int = -1

    private var cameraTextureUniform: Int = -1

    private var cameraTextureId: Int = -1

    fun onCreateInGLThread() {
        // Init camera texture
        val textures = IntArray(1)
        GLES20.glGenTextures(textures.size, textures, 0)
        cameraTextureId = textures[0]

        val textureTarget = GLES11Ext.GL_TEXTURE_EXTERNAL_OES
        GLES20.glBindTexture(textureTarget, cameraTextureId)
        GLES20.glTexParameteri(textureTarget, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE)
        GLES20.glTexParameteri(textureTarget, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE)
        GLES20.glTexParameteri(textureTarget, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR)
        GLES20.glTexParameteri(textureTarget, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR)

        // Init shaders
        val vertexShader = ShaderUtil.loadGLShader(GLES20.GL_VERTEX_SHADER, vertexShaderCode)
        val fragmentShader = ShaderUtil.loadGLShader(GLES20.GL_FRAGMENT_SHADER, fragmentShaderCode)
        if (vertexShader == 0 || fragmentShader == 0) {
            Log.e(TAG, "Failed to compile shader")
            return
        }

        cameraProgram = GLES20.glCreateProgram()
        GLES20.glAttachShader(cameraProgram, vertexShader)
        GLES20.glAttachShader(cameraProgram, fragmentShader)
        GLES20.glLinkProgram(cameraProgram)
        GLES20.glUseProgram(cameraProgram)
        cameraPositionAttrib = GLES20.glGetAttribLocation(cameraProgram, "a_Position")
        cameraTexCoordAttrib = GLES20.glGetAttribLocation(cameraProgram, "a_TexCoord")
        cameraTextureUniform = GLES20.glGetUniformLocation(cameraProgram, "sTexture")

        ShaderUtil.checkGLError("BackgroundRendererCreate")
    }

    fun draw(frame: Frame) {
        if (frame.hasDisplayGeometryChanged()) {
            frame.transformCoordinates2d(Coordinates2d.OPENGL_NORMALIZED_DEVICE_COORDINATES, quadCoords,
                Coordinates2d.TEXTURE_NORMALIZED, quadTexCoords)
        }
        quadTexCoords.position(0)

        // No need to test or write depth, the screen quad has arbitrary depth, and is expected
        // to be drawn first.
        GLES20.glDisable(GLES20.GL_DEPTH_TEST)
        GLES20.glDepthMask(false)

        GLES20.glActiveTexture(GLES20.GL_TEXTURE0)

        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, cameraTextureId)
        GLES20.glUseProgram(cameraProgram)
        GLES20.glUniform1i(cameraTextureUniform, 0)

        // Set the vertex positions and texture coordinates.
        GLES20.glVertexAttribPointer(
            cameraPositionAttrib,
            2,
            GLES20.GL_FLOAT,
            false,
            0,
            quadCoords
        )
        GLES20.glVertexAttribPointer(
            cameraTexCoordAttrib,
            2,
            GLES20.GL_FLOAT,
            false,
            0,
            quadTexCoords
        )
        GLES20.glEnableVertexAttribArray(cameraPositionAttrib)
        GLES20.glEnableVertexAttribArray(cameraTexCoordAttrib)

        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4)

        // Disable vertex arrays
        GLES20.glDisableVertexAttribArray(cameraPositionAttrib)
        GLES20.glDisableVertexAttribArray(cameraTexCoordAttrib)

        // Restore the depth state for further drawing.
        GLES20.glDepthMask(true)
        GLES20.glEnable(GLES20.GL_DEPTH_TEST)

        ShaderUtil.checkGLError("BackgroundRendererDraw")
    }

    fun cameraTextureId() : Int {
        return cameraTextureId
    }
}

class ShaderUtil {
    companion object {
        fun loadGLShader(type: Int, code: String): Int {
            val shader = GLES20.glCreateShader(type)
            GLES20.glShaderSource(shader, code)
            GLES20.glCompileShader(shader)

            val compileStatus = IntArray(1)
            GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, compileStatus, 0)

            if (compileStatus[0] == 0) {
                Log.e("ShadeerUtil", "Error compiling shader: ${GLES20.glGetShaderInfoLog(shader)}")
                GLES20.glDeleteShader(shader)
                return 0
            }

            return shader
        }

        fun checkGLError(label: String) {
            var error = GLES20.glGetError()
            while (error != GLES20.GL_NO_ERROR) {
                Log.e(TAG, "$label: $error")
                error = GLES20.glGetError()
            }
        }
    }
}
