"""Test gs_ws against a tiny local WebSocket server (no ESP32 needed).
Run: python3 -m unittest -v (in tools/)"""
import base64
import hashlib
import socket
import threading
import unittest

from gs_ws import WsClient

MAGIC = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"  # from the WebSocket spec


def fake_server(sock, messages, received):
    """Accept one client, send text frames, read one masked frame back."""
    conn, _ = sock.accept()
    req = b""
    while b"\r\n\r\n" not in req:
        req += conn.recv(1024)
    key = [ln.split(":", 1)[1].strip() for ln in req.decode().split("\r\n")
           if ln.lower().startswith("sec-websocket-key")][0]
    accept = base64.b64encode(hashlib.sha1((key + MAGIC).encode()).digest()).decode()
    conn.sendall(("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\n"
                  f"Connection: Upgrade\r\nSec-WebSocket-Accept: {accept}\r\n\r\n").encode())
    conn.sendall(bytes([0x89, 0]))  # a ping the client must skip (and answer)
    for m in messages:
        data = m.encode()
        conn.sendall(bytes([0x81, len(data)]) + data)  # server frames are not masked
    # read frames from the client: first the pong, then our text
    for _ in range(2):
        b1, b2 = conn.recv(2)
        n = b2 & 0x7F
        mask = conn.recv(4)
        payload = bytes(c ^ mask[i % 4] for i, c in enumerate(conn.recv(n)))
        received.append((b1 & 0x0F, payload.decode()))
    conn.close()


class WsTest(unittest.TestCase):
    def test_read_and_send(self):
        srv = socket.socket()
        srv.bind(("127.0.0.1", 0))
        srv.listen(1)
        port = srv.getsockname()[1]
        got = []
        lines = ["$GS,1,100,hello", "$GS,2,200,world"]
        th = threading.Thread(target=fake_server, args=(srv, lines, got))
        th.start()

        ws = WsClient("127.0.0.1", port)
        self.assertEqual(ws.recv_text(), lines[0])
        self.assertEqual(ws.recv_text(), lines[1])
        ws.send_text("ping")
        th.join(timeout=5)
        ws.close()
        srv.close()
        self.assertEqual(got, [(0xA, ""), (0x1, "ping")])


if __name__ == "__main__":
    unittest.main()
