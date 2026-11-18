"""Very small WebSocket client using only the Python standard library.

Enough for this project: connect, read text frames, send short text
frames ("ping"). No TLS, no fragmented messages, no extensions.
"""
import base64
import os
import socket
import struct


class WsClient:
    def __init__(self, host, port=81, timeout=2.0):
        self.sock = socket.create_connection((host, port), timeout=timeout)
        self.buf = b""
        self._handshake(host, port)

    def _handshake(self, host, port):
        key = base64.b64encode(os.urandom(16)).decode()
        req = (f"GET / HTTP/1.1\r\nHost: {host}:{port}\r\nUpgrade: websocket\r\n"
               f"Connection: Upgrade\r\nSec-WebSocket-Key: {key}\r\n"
               "Sec-WebSocket-Version: 13\r\n\r\n")
        self.sock.sendall(req.encode())
        while b"\r\n\r\n" not in self.buf:
            self.buf += self._recv_some()
        head, _, self.buf = self.buf.partition(b"\r\n\r\n")
        if b" 101 " not in head.split(b"\r\n")[0]:
            raise ConnectionError("server did not accept the WebSocket upgrade")

    def _recv_some(self):
        data = self.sock.recv(4096)
        if not data:
            raise ConnectionError("connection closed")
        return data

    def _read_exact(self, n):
        while len(self.buf) < n:
            self.buf += self._recv_some()
        out, self.buf = self.buf[:n], self.buf[n:]
        return out

    def recv_text(self):
        """Return the next text message. Raises socket.timeout if none came."""
        while True:
            b1, b2 = self._read_exact(2)
            opcode = b1 & 0x0F
            length = b2 & 0x7F
            if length == 126:
                length = struct.unpack(">H", self._read_exact(2))[0]
            elif length == 127:
                length = struct.unpack(">Q", self._read_exact(8))[0]
            mask = self._read_exact(4) if b2 & 0x80 else None
            payload = self._read_exact(length)
            if mask:
                payload = bytes(c ^ mask[i % 4] for i, c in enumerate(payload))
            if opcode == 0x1:              # text
                return payload.decode(errors="replace")
            if opcode == 0x8:              # close
                raise ConnectionError("server closed the WebSocket")
            if opcode == 0x9:              # ping -> answer with pong
                self._send_frame(0xA, payload)

    def send_text(self, text):
        self._send_frame(0x1, text.encode())

    def _send_frame(self, opcode, payload):
        # client frames must be masked (WebSocket rule). Payloads here are short.
        mask = os.urandom(4)
        masked = bytes(c ^ mask[i % 4] for i, c in enumerate(payload))
        self.sock.sendall(bytes([0x80 | opcode, 0x80 | len(payload)]) + mask + masked)

    def close(self):
        self.sock.close()
