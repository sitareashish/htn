#!/usr/bin/env python3
"""HTN chaos client: deliberately misbehave to test server robustness."""
import socket, struct, sys, time, os, random

HOST, PORT = "127.0.0.1", 9100
MAGIC, VER = 0x4854, 1
T_PING, T_ECHO = 1, 3

def frame(t, payload=b""):
    return struct.pack(">HBBI", MAGIC, VER, t, len(payload)) + payload

def conn():
    s = socket.create_connection((HOST, PORT))
    s.settimeout(5)
    return s

def byte_at_a_time():
    s = conn(); pkt = frame(T_ECHO, b"hello-slow")
    for b in pkt:
        s.send(bytes([b])); time.sleep(0.005)
    print("byte_at_a_time reply:", s.recv(256)[8:]); s.close()

def half_close():
    s = conn(); s.send(frame(T_ECHO, b"half"))
    s.shutdown(socket.SHUT_WR)              # stop writing, keep reading
    print("half_close reply:", s.recv(256)[8:]); s.close()

def abrupt_rst():
    s = conn(); s.send(frame(T_PING))
    # SO_LINGER {on=1, timeout=0} => close() sends RST instead of FIN
    s.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("ii", 1, 0))
    s.close(); print("abrupt_rst: sent RST")

def oversized():
    s = conn()
    # claim a payload_len far beyond HTN_MAX_PAYLOAD, send only the header
    s.send(struct.pack(">HBBI", MAGIC, VER, T_ECHO, 10_000_000))
    try: print("oversized: server closed?", s.recv(16))
    except Exception as e: print("oversized: closed:", e)
    s.close()

def garbage():
    s = conn(); s.send(os.urandom(64))
    try: print("garbage: server closed?", s.recv(16))
    except Exception as e: print("garbage: closed:", e)
    s.close()

def slowloris(n=200):
    conns = []
    for _ in range(n):
        try:
            s = conn(); s.send(b"\x48\x54")   # partial header, then stall
            conns.append(s)
        except Exception: break
    print(f"slowloris: opened {len(conns)} stalled conns; server should stay up")
    time.sleep(2)
    for s in conns: s.close()

def flood(n=500):
    ok = 0
    for _ in range(n):
        try:
            s = conn(); s.send(frame(T_PING)); s.recv(16); ok += 1; s.close()
        except Exception: pass
    print(f"flood: {ok}/{n} clean PINGs")

CMDS = {"byte": byte_at_a_time, "half": half_close, "rst": abrupt_rst,
        "oversized": oversized, "garbage": garbage, "slowloris": slowloris,
        "flood": flood}

if __name__ == "__main__":
    which = sys.argv[1] if len(sys.argv) > 1 else "all"
    if which == "all":
        for name, fn in CMDS.items():
            print(f"== {name} =="); fn()
    else:
        CMDS[which]()
