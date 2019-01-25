#!/usr/bin/env python3
import sys
import os
from http.server import HTTPServer
from http.server import BaseHTTPRequestHandler
from http import HTTPStatus

class Server(BaseHTTPRequestHandler):
    def do_HEAD(self):
            return

    def do_GET(self):
        self.send_response(HTTPStatus.OK)
        self.end_headers()

    def do_POST(self):
        self.send_response(HTTPStatus.OK)
        self.end_headers()

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: %s <port> <max_wait_time>" % sys.argv[0])
        sys.exit(1)
    httpd = HTTPServer(('localhost', int(sys.argv[1])), Server)
    httpd.timeout = int(sys.argv[2])
    try:
        httpd.handle_request()
    except KeyboardInterrupt:
        pass
    httpd.server_close()
