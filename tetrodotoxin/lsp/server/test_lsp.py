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


def send_did_open(conn, uri, source_text):
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "method": "textDocument/didOpen",
        "params": {
            "textDocument": {
                "uri": uri,
                "languageId": "tetrodotoxin",
                "version": 1,
                "text": source_text,
            },
        },
    }))
    time.sleep(0.1)


def send_semantic_tokens(conn, uri, request_id):
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "id": request_id,
        "method": "textDocument/semanticTokens/full",
        "params": {"textDocument": {"uri": uri}},
    }))
    return read_lsp_response(conn, timeout=10.0)


def semantic_token_texts(source_text, data):
    lines = source_text.splitlines(keepends=True)
    line = 0
    column = 0
    tokens = []

    for i in range(0, len(data), 5):
        delta_line, delta_start, length, token_type, modifiers = data[i:i + 5]
        line += delta_line
        if delta_line:
            column = delta_start
        else:
            column += delta_start

        if line < len(lines):
            tokens.append((lines[line][column:column + length], token_type))

    return tokens


def run_test():
    failures = []

    def check(condition, message):
        if condition:
            print(f"  [OK] {message}")
        else:
            print(f"  [FAIL] {message}")
            failures.append(message)

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
        return 1

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
    caps = {}
    if init_resp:
        caps = init_resp.get("result", {}).get("capabilities", {})
        info = init_resp.get("result", {}).get("serverInfo", {})
        print(f"  Server: {info.get('name')} v{info.get('version')}")
        print(f"  Capabilities: {list(caps.keys())}")
        check("semanticTokensProvider" in caps,
              "server advertises semantic tokens")
        semantic_provider = caps.get("semanticTokensProvider", {})
        legend = semantic_provider.get("legend", {})
        check("keyword" in legend.get("tokenTypes", []),
              "semantic token legend includes keyword")
        check(bool(semantic_provider.get("full")),
              "server supports full semantic token requests")
    else:
        print("  ERROR: no initialize response")
        failures.append("initialize response")

    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "method": "initialized",
        "params": {},
    }))
    time.sleep(0.1)

    print("\n--- Semantic tokens: Library/default dialects ---")
    library_source = (
        "dialect : Library;\n"
        "@public func run[] -> Count {\n"
        "  while (true) {\n"
        "    continue;\n"
        "  }\n"
        "  return 0;\n"
        "}\n"
    )
    library_uri = "file:///semantic-library.ttx"
    send_did_open(conn, library_uri, library_source)
    library_resp = send_semantic_tokens(conn, library_uri, 20)
    library_data = library_resp.get("result", {}).get("data", []) if library_resp else []
    library_texts = [text for text, _ in semantic_token_texts(
        library_source, library_data)]
    check(len(library_data) > 0, "Library document returns semantic tokens")
    check("while" in library_texts and "continue" in library_texts,
          "Library document highlights loop-control keywords")

    no_dialect_source = (
        "@public func draft[] -> Count {\n"
        "  @stack label : Text = \"Icon \\\"Preview\\\"\";\n"
        "  if (true) {\n"
        "    continue;\n"
        "  }\n"
        "  return 0;\n"
        "}\n"
    )
    no_dialect_uri = "file:///semantic-draft.ttx"
    send_did_open(conn, no_dialect_uri, no_dialect_source)
    no_dialect_resp = send_semantic_tokens(conn, no_dialect_uri, 21)
    no_dialect_data = no_dialect_resp.get("result", {}).get("data", []) if no_dialect_resp else []
    no_dialect_texts = [text for text, _ in semantic_token_texts(
        no_dialect_source, no_dialect_data)]
    check(len(no_dialect_data) > 0, "no-dialect document returns semantic tokens")
    check("if" in no_dialect_texts and "continue" in no_dialect_texts,
          "missing dialect defaults to Library highlighting")
    check("\"Icon \\\"Preview\\\"\"" in no_dialect_texts,
          "document sync decodes escaped string text")

    print("\n--- Semantic tokens: Shader dialect filtering ---")
    shader_source = (
        "dialect : Shader;\n"
        "@public func main[] -> Count {\n"
        "  if (true) {\n"
        "    continue;\n"
        "  }\n"
        "  return 0;\n"
        "}\n"
    )
    shader_uri = "file:///semantic-shader.ttx"
    send_did_open(conn, shader_uri, shader_source)
    shader_resp = send_semantic_tokens(conn, shader_uri, 22)
    shader_data = shader_resp.get("result", {}).get("data", []) if shader_resp else []
    shader_texts = [text for text, _ in semantic_token_texts(
        shader_source, shader_data)]
    check(len(shader_data) > 0, "Shader document returns semantic tokens")
    check("return" in shader_texts, "Shader document keeps shared control keywords")
    check("if" not in shader_texts and "continue" not in shader_texts,
          "Shader document filters Library-only control keywords")

    print("\n--- Cancellation ---")
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "method": "$/cancelRequest",
        "params": {"id": 999},
    }))
    cancel_probe_resp = send_semantic_tokens(conn, shader_uri, 23)
    check(cancel_probe_resp is not None and "result" in cancel_probe_resp,
          "cancel notification keeps server responsive")

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

    print("\n--- Round-trip: apps/splash_screen.ttx ---")
    splash_path = os.path.join(REPO_ROOT, "apps", "splash_screen.ttx")
    with open(splash_path, "r", encoding="utf-8") as f:
        splash_source = f.read()

    splash_formatted = send_format(conn, splash_source, "splash_screen.ttx")
    if splash_formatted is not None:
        if splash_formatted == splash_source:
            print("  [OK] splash_screen.ttx is unchanged after formatting")
        else:
            print("  [DIFF] splash_screen.ttx changed after formatting:")
            src_lines = splash_source.splitlines()
            fmt_lines = splash_formatted.splitlines()
            for i, (a, b) in enumerate(zip(src_lines, fmt_lines), 1):
                if a != b:
                    print(f"    line {i}:")
                    print(f"      before: {repr(a)}")
                    print(f"      after:  {repr(b)}")
            if len(src_lines) != len(fmt_lines):
                print(f"  line count: {len(src_lines)} → {len(fmt_lines)}")

    print("\n--- Round-trip: invalid source ---")
    invalid_source = "dialect : Library;\n\n$\n"
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

    if failures:
        print("\nFailures:")
        for failure in failures:
            print(f"  - {failure}")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(run_test())
