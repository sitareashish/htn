#!/usr/bin/env python3
"""Minimal HTN protocol client. Sends PING and ECHO, verifies responses."""
import socket, struct, sys

MAGIC, VER = 0x4854, 1
PING, PONG, ECHO = 1, 2, 3

def frame(t, payload=b""):
    return struct.pack(">HBBI", MAGIC, VER, t, len(payload)) + payload

def read_frame(s):
    hdr = b""
    while len(hdr) < 8:
        b = s.recv(8 - len(hdr))
        if not b: raise EOFError
        hdr += b
    magic, ver, t, plen = struct.unpack(">HBBI", hdr)
    pay = b""
    while len(pay) < plen:
        b = s.recv(plen - len(pay))
        if not b: raise EOFError
        pay += b
    return t, pay

host = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
port = int(sys.argv[2]) if len(sys.argv) > 2 else 9100
s = socket.create_connection((host, port)); s.settimeout(5)

s.sendall(frame(PING))
t, _ = read_frame(s)
assert t == PONG, f"expected PONG got {t}"
print("PING -> PONG ok")

msg = b"hello htn protocol"
s.sendall(frame(ECHO, msg))
t, pay = read_frame(s)
assert t == ECHO and pay == msg, f"echo mismatch: {t} {pay!r}"
print(f"ECHO -> {pay!r} ok")

# Split-read torture: send an ECHO frame one byte at a time.
import time
f = frame(ECHO, b"dribble")
for byte in f:
    s.sendall(bytes([byte])); time.sleep(0.05)
t, pay = read_frame(s)
assert t == ECHO and pay == b"dribble", "split-read framing failed"
print("split-read ECHO ok")
print("ALL OK")
