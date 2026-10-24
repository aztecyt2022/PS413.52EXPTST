#!/usr/bin/env python3
"""Network loopback tests against a disposable mock FTP daemon; never PS4."""
import importlib.util
import socket
import threading
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1]/"tools"/"ftp-control.py"
spec = importlib.util.spec_from_file_location("ftp_control",SCRIPT)
ftp = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ftp)


class FakeServer:
    def __init__(self, banner="220 FTPS4 Server ready."):
        self.banner = banner
        self.socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
        self.socket.bind(("127.0.0.1",0))
        self.port = self.socket.getsockname()[1]
        self.requests = []
        self.active = True
        self.socket.listen(2)
        self.thread = threading.Thread(target=self.run,daemon=True)
        self.thread.start()

    def run(self):
        self.socket.settimeout(0.5)
        while self.active:
            try:
                conn,_ = self.socket.accept()
            except (OSError,socket.timeout):
                continue
            with conn:
                conn.settimeout(2)
                conn.sendall((self.banner+"\r\n").encode())
                try:
                    request=conn.recv(512).decode("ascii",errors="ignore").strip()
                except OSError:
                    continue
                if request:
                    self.requests.append(request)
                if request == "SHUTDOWN":
                    conn.sendall(b"200 Shutting down...\r\n")
                    self.active=False
                    self.socket.close()

    def close(self):
        self.active=False
        self.socket.close()
        self.thread.join(timeout=3)


class FTPTests(unittest.TestCase):
    def test_private_ip_only(self):
        self.assertEqual(ftp.safe_local_ip("192.168.1.15"),"192.168.1.15")
        self.assertEqual(ftp.safe_local_ip("10.0.0.7"),"10.0.0.7")
        for address in ["8.8.8.8","127.0.0.1","0.0.0.0","example.com","ff02::1"]:
            with self.assertRaises(Exception):
                ftp.safe_local_ip(address)

    def test_status_only_never_stops(self):
        mock=FakeServer()
        try:
            state,_=ftp.probe("127.0.0.1",mock.port)
            self.assertEqual(state,"on")
            self.assertEqual(mock.requests,[])
        finally:
            mock.close()

    def test_shutdown_closes_listener(self):
        mock=FakeServer()
        try:
            ok,detail=ftp.stop("127.0.0.1",mock.port,verify_seconds=4)
            self.assertTrue(ok,detail)
            self.assertEqual(mock.requests,["SHUTDOWN"])
        finally:
            mock.close()

    def test_wrong_server_never_shutdown(self):
        mock=FakeServer(banner="220 unrelated FTP server")
        try:
            ok,msg=ftp.stop("127.0.0.1",mock.port)
            self.assertFalse(ok)
            self.assertIn("different FTP server",msg)
            self.assertEqual(mock.requests,[])
        finally:
            mock.close()

    def test_closed_port_reports_off(self):
        s=socket.socket()
        s.bind(("127.0.0.1",0));port=s.getsockname()[1];s.close()
        state,_=ftp.probe("127.0.0.1",port)
        self.assertEqual(state,"off")

if __name__ == "__main__":
    unittest.main()