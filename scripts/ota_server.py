#!/usr/bin/env python3
"""
Simple HTTP server for OTA firmware testing
Usage: python ota_server.py
"""

import http.server
import socketserver
import os

PORT = 8070
FIRMWARE_DIR = "../build"

class OTAHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=FIRMWARE_DIR, **kwargs)
    
    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        super().end_headers()

if __name__ == "__main__":
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    
    print(f"Starting OTA server on port {PORT}")
    print(f"Serving files from: {os.path.abspath(FIRMWARE_DIR)}")
    print(f"Firmware URL: http://YOUR_IP:{PORT}/esp32-secure-ota.bin")
    print("Press Ctrl+C to stop")
    
    with socketserver.TCPServer(("", PORT), OTAHandler) as httpd:
        httpd.serve_forever()
