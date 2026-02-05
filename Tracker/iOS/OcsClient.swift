import Network
import UIKit

class OscClient {
    private let udpConnection: NWConnection
    private let udpQueue: DispatchQueue

    init (address: String, port: UInt16) {
        udpConnection = NWConnection(host: NWEndpoint.Host(address), port: NWEndpoint.Port(rawValue: port)!, using: .udp)
        udpQueue = DispatchQueue(label: "udp.sending.queue")
        udpConnection.start(queue: udpQueue)
    }

    public func send(path: String, data: [Float]) {
        var bytes = path.oscString()

        let typeTag = "," + String(repeating: "f", count: data.count)
        bytes += typeTag.oscString()

        for value in data {
            let bigEndian = value.bitPattern.bigEndian
            bytes.append(UInt8((bigEndian >> 0) & 0xFF))
            bytes.append(UInt8((bigEndian >> 8) & 0xFF))
            bytes.append(UInt8((bigEndian >> 16) & 0xFF))
            bytes.append(UInt8((bigEndian >> 24) & 0xFF))
        }

        udpQueue.async { [weak self] in 
            self?.udpConnection.send(content: Data(bytes), completion: .contentProcessed({ _ in }))
        }
    }
}

extension String {
    func oscString() -> [UInt8] {
        var bytes: [UInt8] = self.utf8.map { UInt8($0)}
        bytes.append(0)
        
        while bytes.count % 4 != 0 {
            bytes.append(0)
        }

        return bytes
    }
}
