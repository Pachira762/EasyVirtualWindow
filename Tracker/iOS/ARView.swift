import SwiftUI
import UIKit
import ARKit
import RealityKit
import Network

struct ARViewWrapper: UIViewRepresentable {
    @ObservedObject var trackingData: TrackingData
    
    func makeUIView(context: Context) -> CustomARView {
        let arView = CustomARView(frame: .zero)
        arView.trackingData = trackingData

        return arView
    }

    func updateUIView(_ arView: CustomARView, context: Context) {
        if (context.environment.scenePhase == .active) {
            arView.startSession()
        }else{
            arView.stopSession()
        }
    }
}

class CustomARView: ARView, ARSessionDelegate {
    weak var trackingData: TrackingData?
    
    private let oscClient: OscClient = OscClient(address:"192.168.1.28", port:11125)
    private var sessionStarted: Bool = false

    required init(frame frameRect: CGRect) {
        super.init(frame: frameRect)
        startSession()
    }
    
    required init?(coder decoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    func startSession() {
        if (sessionStarted) {
            return
        }

        guard ARFaceTrackingConfiguration.isSupported else {
            print("このデバイスはFace Trackingをサポートしていません (FaceID搭載機が必要です)")
            return
        }

        session.delegate = self
        automaticallyConfigureSession = false

        let configuration = ARFaceTrackingConfiguration()
        session.run(configuration, options: [.resetTracking, .removeExistingAnchors])

        sessionStarted = true
    }

    func stopSession() {
        if (sessionStarted) {
            session.pause()
            sessionStarted=false
        }
    }
    
    func session(_ session: ARSession, didUpdate anchors: [ARAnchor]) {
        let DEVICE_ID:Int32 = 2

        guard let faceAnchor = anchors.first as? ARFaceAnchor else { 
            oscClient.send(path:"/face-tracker/\(DEVICE_ID)/lost", data:[])
            return 
        }

        let position =  simd_mul(faceAnchor.transform, simd_float4(0.0, 0.04, 0.02, 1.0))
        let x = -100.0 * position.z
        let y = -100.0 * position.x
        let z = 100.0 * position.y

        oscClient.send(path:"/face-tracker/\(DEVICE_ID)/eye-position", data:[x, y, z])

        DispatchQueue.main.async {
            self.trackingData?.setFacePosition(x:x, y:y, z:z)
        }
    }
}
