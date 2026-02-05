package com.example.facetracker

import kotlinx.coroutines.DelicateCoroutinesApi
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress
import java.nio.ByteBuffer
import java.nio.ByteOrder


class OscClient (address: String, private val port: Int) {
    private val socket: DatagramSocket = DatagramSocket().apply {
        broadcast = true
    }

    private val address: InetAddress = InetAddress.getByName(address)

    fun send(path: String) {
        val output = mutableListOf<Byte>()
        output.addAll(path.toOscString())
        output.addAll(",".toOscString())
        send(output.toByteArray())
    }

    fun send(path: String, data: FloatArray) {
        val output = mutableListOf<Byte>()
        output.addAll(path.toOscString())

        val typeTag = "," + "f".repeat(data.size)
        output.addAll(typeTag.toOscString())

        val dataBuffer = ByteBuffer.allocate(data.size * 4).apply {
            order(ByteOrder.BIG_ENDIAN)
            for (value in data) {
                putFloat(value)
            }
        }
        output.addAll(dataBuffer.array().toList())

        val bytes = output.toByteArray()
        send(bytes)
    }

    private fun send(bytes: ByteArray) {
        @OptIn(DelicateCoroutinesApi::class)
        GlobalScope.launch(Dispatchers.IO) {
            val packet = DatagramPacket(bytes, bytes.size, address, port)
            socket.send(packet)
        }
    }
}

fun String.toOscString(): MutableList<Byte> {
    val bytes = this.toByteArray(Charsets.UTF_8).toMutableList().apply{
        add(0)
    }

    while (bytes.size % 4 != 0) {
        bytes.add(0)
    }

    return bytes
}