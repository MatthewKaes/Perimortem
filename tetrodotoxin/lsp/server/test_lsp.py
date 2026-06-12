#!/usr/bin/env python3
"""
Perimortem Engine
Copyright © Matt Kaes

Local LSP server test harness.

Creates a Unix socket server to test the TTX language server with a few basic
commands without having to do a full .visx build + restart of VS Code.

Usage:
    python3 tetrodotoxin/lsp/server/test_lsp.py
"""

import base64
import json
import os
import socket
import subprocess
import sys
import threading
import time

SOCKET_PATH = "/tmp/ttx_lsp_test.sock"
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "..", "..", ".."))
BINARY = os.path.join(REPO_ROOT, ".bin/bin/tetrodotoxin/lsp/server/ttx-lang-server")


def lsp_frame(obj):
    body = json.dumps(obj)
    return f"Content-Length: {len(body)}\r\n\r\n{body}".encode()


def drain_output(proc, label):
    for line in proc.stderr:
        print(f"[{label}] {line}", end="", flush=True)


def read_lsp_response(conn, timeout=5.0):
    """Read one complete LSP response (header + body). Returns the parsed JSON
    body dict, or None on timeout."""
    conn.settimeout(timeout)
    buf = b""
    try:
        while True:
            chunk = conn.recv(4096)
            if not chunk:
                return None
            buf += chunk
            if b"\r\n\r\n" in buf:
                header, _, rest = buf.partition(b"\r\n\r\n")
                content_length = 0
                for part in header.split(b"\r\n"):
                    if part.lower().startswith(b"content-length:"):
                        content_length = int(part.split(b":")[1].strip())
                while len(rest) < content_length:
                    chunk = conn.recv(4096)
                    if not chunk:
                        break
                    rest += chunk
                return json.loads(rest[:content_length].decode())
    except socket.timeout:
        return None


def send_format(conn, source_text, name):
    """Send a format request and return the decoded formatted text, or None on error."""
    encoded = base64.b64encode(source_text.encode("utf-8")).decode()
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "id": 10,
        "method": "format",
        "params": {"source": encoded, "name": name},
    }))
    resp = read_lsp_response(conn, timeout=10.0)
    if resp is None:
        print(f"  ERROR: no response for {name}")
        return None
    if "error" in resp:
        print(f"  ERROR from server for {name}: {resp['error']}")
        return None
    encoded_doc = resp.get("result", {}).get("document", "")
    if not encoded_doc:
        print(f"  ERROR: response for {name} has no 'document' field")
        return None
    return base64.b64decode(encoded_doc.encode()).decode("utf-8")


def run_test():
    if os.path.exists(SOCKET_PATH):
        os.unlink(SOCKET_PATH)

    server_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_sock.bind(SOCKET_PATH)
    server_sock.listen(1)
    server_sock.settimeout(5)
    print(f"Socket ready at {SOCKET_PATH}")

    env = os.environ.copy()
    if "--asan" in sys.argv:
        asan_path = subprocess.run(
            ["clang", "-print-file-name=libasan.so"],
            capture_output=True, text=True).stdout.strip()
        if asan_path and os.path.exists(asan_path):
            env["LD_PRELOAD"] = asan_path
            print(f"ASAN: {asan_path}")

    proc = subprocess.Popen(
        [BINARY, f"--pipe={SOCKET_PATH}"],
        stderr=subprocess.PIPE,
        text=True,
        env=env,
    )
    print(f"Server launched (pid={proc.pid})")

    drain_thread = threading.Thread(
        target=drain_output, args=(proc, "server"), daemon=True)
    drain_thread.start()

    try:
        conn, _ = server_sock.accept()
        print("Server connected to socket.")
    except socket.timeout:
        print("ERROR: server did not connect within 5s")
        proc.terminate()
        drain_thread.join(timeout=2)
        return

    print("\n--- Sending initialize ---")
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "id": 1,
        "method": "initialize",
        "params": {
            "processId": os.getpid(),
            "clientInfo": {"name": "ttx-test"},
            "capabilities": {},
        },
    }))

    init_resp = read_lsp_response(conn)
    if init_resp:
        caps = init_resp.get("result", {}).get("capabilities", {})
        info = init_resp.get("result", {}).get("serverInfo", {})
        print(f"  Server: {info.get('name')} v{info.get('version')}")
        print(f"  Capabilities: {list(caps.keys())}")
    else:
        print("  ERROR: no initialize response")

    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "method": "initialized",
        "params": {},
    }))
    time.sleep(0.1)

    print("\n--- Round-trip: validation/data/ttx/png.ttx ---")
    png_path = os.path.join(REPO_ROOT, "validation", "data", "ttx", "png.ttx")
    with open(png_path, "r", encoding="utf-8") as f:
        png_source = f.read()

    png_formatted = send_format(conn, png_source, "png.ttx")
    if png_formatted is not None:
        if png_formatted == png_source:
            print("  [OK] png.ttx is unchanged after formatting")
        else:
            print("  [DIFF] png.ttx changed after formatting:")
            src_lines = png_source.splitlines()
            fmt_lines = png_formatted.splitlines()
            for i, (a, b) in enumerate(zip(src_lines, fmt_lines), 1):
                if a != b:
                    print(f"    line {i}:")
                    print(f"      before: {repr(a)}")
                    print(f"      after:  {repr(b)}")
            if len(src_lines) != len(fmt_lines):
                print(f"  line count: {len(src_lines)} → {len(fmt_lines)}")

    print("\n--- Round-trip: apps/shaders/icon.ttx ---")
    icon_path = os.path.join(REPO_ROOT, "apps", "shaders", "icon.ttx")
    with open(icon_path, "r", encoding="utf-8") as f:
        icon_source = f.read()

    icon_formatted = send_format(conn, icon_source, "icon.ttx")
    if icon_formatted is not None:
        if icon_formatted == icon_source:
            print("  [OK] icon.ttx is unchanged after formatting")
        else:
            print("  [DIFF] icon.ttx changed after formatting:")
            src_lines = icon_source.splitlines()
            fmt_lines = icon_formatted.splitlines()
            for i, (a, b) in enumerate(zip(src_lines, fmt_lines), 1):
                if a != b:
                    print(f"    line {i}:")
                    print(f"      before: {repr(a)}")
                    print(f"      after:  {repr(b)}")
            if len(src_lines) != len(fmt_lines):
                print(f"  line count: {len(src_lines)} → {len(fmt_lines)}")

    print("\n--- Round-trip: invalid source ---")
    invalid_source = "package : Library;\n\n$\n"
    invalid_formatted = send_format(conn, invalid_source, "invalid.ttx")
    if invalid_formatted is not None:
        if invalid_formatted == invalid_source:
            print("  [OK] invalid source is unchanged after formatting")
        else:
            print("  [DIFF] invalid source changed after formatting:")
            print(f"      before: {repr(invalid_source)}")
            print(f"      after:  {repr(invalid_formatted)}")

    exit_code = proc.poll()
    if exit_code is None:
        print("\nServer still running — shutting down.")
        conn.close()
        proc.terminate()
        proc.wait(timeout=3)
    else:
        print(f"\nServer exited with code {exit_code}")

    drain_thread.join(timeout=2)

    try:
        conn.close()
    except Exception:
        pass
    server_sock.close()
    if os.path.exists(SOCKET_PATH):
        os.unlink(SOCKET_PATH)


if __name__ == "__main__":
    run_test()
