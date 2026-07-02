#!/usr/bin/env python3
"""
HTTPS firmware server for OTA testing.

Usage:
    python scripts/ota_server.py --cert path/to/server.crt --key path/to/server.key
"""

import argparse
import http.server
import os
import ssl
import socketserver


PORT = 8070
FIRMWARE_DIR = "../build"
FIRMWARE_NAME = "esp32-secure-ota.bin"


class OTAHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=FIRMWARE_DIR, **kwargs)


def parse_args():
    parser = argparse.ArgumentParser(description="Serve OTA firmware over HTTPS")
    parser.add_argument("--host", default="", help="Bind host; default binds all interfaces")
    parser.add_argument("--port", type=int, default=PORT, help="HTTPS port")
    parser.add_argument("--cert", required=True, help="Server certificate PEM path")
    parser.add_argument("--key", required=True, help="Server private key PEM path")
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    script_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(script_dir)

    cert_path = os.path.abspath(args.cert)
    key_path = os.path.abspath(args.key)
    firmware_path = os.path.abspath(os.path.join(FIRMWARE_DIR, FIRMWARE_NAME))

    if not os.path.exists(cert_path):
        raise SystemExit(f"Certificate not found: {cert_path}")
    if not os.path.exists(key_path):
        raise SystemExit(f"Private key not found: {key_path}")
    if not os.path.exists(firmware_path):
        print(f"Warning: firmware binary not found yet: {firmware_path}")

    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(certfile=cert_path, keyfile=key_path)

    with socketserver.TCPServer((args.host, args.port), OTAHandler) as httpd:
        httpd.socket = context.wrap_socket(httpd.socket, server_side=True)
        print(f"Starting HTTPS OTA server on port {args.port}")
        print(f"Serving files from: {os.path.abspath(FIRMWARE_DIR)}")
        print(f"Firmware URL: https://YOUR_IP:{args.port}/{FIRMWARE_NAME}")
        print("Make sure the ESP32 trusts the certificate chain before enabling OTA.")
        print("Press Ctrl+C to stop")
        httpd.serve_forever()
