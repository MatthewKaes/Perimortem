#!/usr/bin/env python3
"""
Perimortem Engine
Copyright © Matt Kaes

Bounded Puffer LSP stress harness.

This starts a fresh Puffer process for each session and sends a mix of
formatting, synchronization, semantic-token, and ignored-notification traffic.
The goal is not a language conformance suite; it is a cheap crash and framing
probe for the server boundary that VSCode exercises.

Usage:
    python3 tetrodotoxin/puffer/lsp/stress_lsp.py
    python3 tetrodotoxin/puffer/lsp/stress_lsp.py --sessions=10 --requests=50
"""

import argparse
import os
import socket
import subprocess
import sys
import threading
import time

from test_lsp import (
    BINARY,
    REPO_ROOT,
    lsp_frame,
    read_lsp_response,
    send_did_open,
    send_format,
    send_semantic_tokens,
)


def parse_args():
    parser = argparse.ArgumentParser(description="Stress Puffer LSP mode.")
    parser.add_argument("--sessions", type=int, default=5)
    parser.add_argument("--requests", type=int, default=25)
    return parser.parse_args()


def source_for(iteration):
    if iteration % 3 == 0:
        return (
            "dialect : Library;\n"
            "public func run[] -> Count {\n"
            "  if (true) {\n"
            "    return 1;\n"
            "  }\n"
            "  return 0;\n"
            "}\n"
        )
    if iteration % 3 == 1:
        return (
            "dialect : Shader;\n"
            "public func main[] -> Count {\n"
            "  return 0;\n"
            "}\n"
        )
    with open(
        os.path.join(REPO_ROOT, "apps", "splash_screen.ttx"),
        "r",
        encoding="utf-8") as source_file:
        return source_file.read()


def launch_server(socket_path):
    if os.path.exists(socket_path):
        os.unlink(socket_path)

    server_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server_socket.bind(socket_path)
    server_socket.listen(1)
    server_socket.settimeout(5)

    proc = subprocess.Popen(
        [BINARY, f"--pipe={socket_path}"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True)
    output_tail = []
    stdout_thread = threading.Thread(
        target=drain_output, args=(proc.stdout, output_tail), daemon=True)
    stderr_thread = threading.Thread(
        target=drain_output, args=(proc.stderr, output_tail), daemon=True)
    stdout_thread.start()
    stderr_thread.start()

    try:
        conn, _ = server_socket.accept()
    except socket.timeout:
        proc.terminate()
        proc.wait(timeout=3)
        raise RuntimeError("server did not connect to the LSP socket")

    return server_socket, conn, proc, stdout_thread, stderr_thread, output_tail


def drain_output(stream, output_tail):
    for line in stream:
        output_tail.append(line.rstrip())
        if len(output_tail) > 50:
            output_tail.pop(0)


def stop_server(
        socket_path,
        server_socket,
        conn,
        proc,
        stdout_thread,
        stderr_thread):
    try:
        conn.close()
    except OSError:
        pass

    try:
        server_socket.close()
    except OSError:
        pass

    if proc.poll() is None:
        proc.terminate()
        proc.wait(timeout=3)

    stdout_thread.join(timeout=2)
    stderr_thread.join(timeout=2)

    if os.path.exists(socket_path):
        os.unlink(socket_path)


def initialize(conn):
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "id": 1,
        "method": "initialize",
        "params": {
            "processId": os.getpid(),
            "clientInfo": {"name": "puffer-stress"},
            "capabilities": {},
        },
    }))
    response = read_lsp_response(conn, timeout=10.0)
    if response is None or "result" not in response:
        raise RuntimeError("initialize did not return a result")

    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "method": "initialized",
        "params": {},
    }))


def exercise_session(session, request_count):
    socket_path = f"/tmp/puffer_lsp_stress_{os.getpid()}_{session}.sock"
    (
        server_socket,
        conn,
        proc,
        stdout_thread,
        stderr_thread,
        output_tail,
    ) = launch_server(socket_path)

    try:
        initialize(conn)

        for i in range(request_count):
            source = source_for(i)
            uri = f"file:///puffer-stress-{session}-{i}.ttx"
            send_did_open(conn, uri, source)

            if send_semantic_tokens(conn, uri, 1000 + i) is None:
                raise RuntimeError(f"semantic tokens timed out in session {session}")

            if send_format(conn, source, uri) is None:
                raise RuntimeError(f"format timed out in session {session}")

            conn.sendall(lsp_frame({
                "jsonrpc": "2.0",
                "method": "$/cancelRequest",
                "params": {"id": 1000 + i},
            }))

            exit_code = proc.poll()
            if exit_code is not None:
                raise RuntimeError(
                    f"server exited early in session {session}: {exit_code}")

            if i % 10 == 0:
                print(
                    f"  session {session}: {i + 1}/{request_count}",
                    flush=True)
    except RuntimeError as error:
        if output_tail:
            tail = "\n".join(output_tail)
            raise RuntimeError(f"{error}\nserver output tail:\n{tail}")
        raise
    finally:
        stop_server(
            socket_path,
            server_socket,
            conn,
            proc,
            stdout_thread,
            stderr_thread)


def main():
    args = parse_args()
    start = time.monotonic()

    print(
        f"Stress testing {BINARY} "
        f"({args.sessions} sessions x {args.requests} requests)")

    for session in range(args.sessions):
        exercise_session(session, args.requests)

    elapsed = time.monotonic() - start
    print(f"OK: completed in {elapsed:.2f}s")
    return 0


if __name__ == "__main__":
    sys.exit(main())
