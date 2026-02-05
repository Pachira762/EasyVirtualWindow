import SwiftUI

final class TrackingData: ObservableObject {
    @Published var facePosition:Array<Float> = [0.0, 0.0, 0.0]

    public func setFacePosition(x: Float, y: Float, z: Float) {
        facePosition[0] = x
        facePosition[1] = y
        facePosition[2] = z
    }
}

struct ContentView: View {
    @StateObject private var trackingData: TrackingData = TrackingData()

    var body: some View {
        ZStack {
            ARViewWrapper(trackingData: trackingData)
                .ignoresSafeArea(edges: [.bottom])
            VStack {
                Text("X = \(trackingData.facePosition[0])")
                Text("Y = \(trackingData.facePosition[1])")
                Text("Z = \(trackingData.facePosition[2])")
            }
        }
    }
}
