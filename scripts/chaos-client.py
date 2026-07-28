#!/usr/bin/env python3
"""Dribble one byte per second and verify each is echoed.
Exposes edge-triggered 'read once and stop' bugs.
Usage: ./scripts/chaos-client.py [host] [port] [message]
"""
import socket, sys, time

host = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
port = int(sys.argv[2]) if len(sys.argv) > 2 else 9100
msg  = (sys.argv[3] if len(sys.argv) > 3 else "hello-edge-triggered").encode()

s = socket.create_connection((host, port))
s.settimeout(5.0)
got = bytearray()
for i, b in enumerate(msg):
    s.sendall(bytes([b]))
    time.sleep(1.0)
    chunk = s.recv(64)
    if not chunk:
        print(f"FAIL: connection closed after byte {i} ({b!r})")
        sys.exit(1)
    got.extend(chunk)
    print(f"sent {b!r} -> echoed {chunk!r}")
s.close()

if bytes(got) == msg:
    print(f"OK: all {len(msg)} bytes echoed correctly")
    sys.exit(0)
print(f"FAIL: expected {msg!r}, got {bytes(got)!r}")
sys.exit(1)
