#!/usr/bin/env python3
"""
Perimortem Engine
Copyright © Matt Kaes

Local LSP server test harness.

Creates a Unix socket server to test the TTX language server with a few basic
commands without having to do a full .visx build + restart of VS Code.

Usage:
    python3 puffer/lsp/test_lsp.py
"""

import json
import os
import socket
import subprocess
import sys
import threading
import time

SOCKET_PATH = "/tmp/ttx_lsp_test.sock"
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))
BINARY = os.environ.get(
    "PUFFER_BINARY",
    os.path.join(REPO_ROOT, ".bin/bin/puffer/puffer"))


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
    """Open and format one document through the standard LSP request."""
    uri = f"file:///{name}"
    send_did_open(conn, uri, source_text)
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "id": 10,
        "method": "textDocument/formatting",
        "params": {
            "textDocument": {"uri": uri},
            "options": {"tabSize": 2, "insertSpaces": True},
        },
    }))
    resp = read_lsp_response(conn, timeout=10.0)
    if resp is None:
        print(f"  ERROR: no response for {name}")
        return None
    if "error" in resp:
        print(f"  ERROR from server for {name}: {resp['error']}")
        return None
    edits = resp.get("result", [])
    if len(edits) != 1 or "newText" not in edits[0]:
        print(f"  ERROR: response for {name} has no complete document edit")
        return None
    return edits[0]["newText"]


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
    return read_lsp_response(conn, timeout=10.0)


def send_did_change(conn, uri, source_text, version):
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "method": "textDocument/didChange",
        "params": {
            "textDocument": {"uri": uri, "version": version},
            "contentChanges": [{"text": source_text}],
        },
    }))
    return read_lsp_response(conn, timeout=10.0)


def send_hover(conn, uri, source_text, needle, request_id, start=0):
    offset = source_text.index(needle, start)
    line = source_text.count("\n", 0, offset)
    line_start = source_text.rfind("\n", 0, offset) + 1
    character = len(source_text[line_start:offset].encode("utf-16-le")) // 2
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "id": request_id,
        "method": "textDocument/hover",
        "params": {
            "textDocument": {"uri": uri},
            "position": {"line": line, "character": character},
        },
    }))
    return read_lsp_response(conn, timeout=10.0)


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
        [
            BINARY,
            f"--pipe={SOCKET_PATH}",
            "--packages-root=" + os.path.join(
                REPO_ROOT, "packages", "ttx"),
        ],
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
        check(bool(caps.get("hoverProvider")),
              "server advertises semantic hover")
        check(bool(caps.get("documentFormattingProvider")),
              "server advertises document formatting")
    else:
        print("  ERROR: no initialize response")
        failures.append("initialize response")

    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "method": "initialized",
        "params": {},
    }))
    time.sleep(0.1)

    print("\n--- Package session: cross-source and System ABI ---")
    package_root = os.path.join(
        REPO_ROOT, "validation", "data", "ttx", "package_session")
    helper_path = os.path.join(package_root, "helper.ttx")
    main_path = os.path.join(package_root, "main.ttx")
    package_path = os.path.join(package_root, "package.ttx")
    with open(helper_path, "r", encoding="utf-8") as f:
        helper_source = f.read()
    with open(main_path, "r", encoding="utf-8") as f:
        main_source = f.read()
    with open(package_path, "r", encoding="utf-8") as f:
        package_source = f.read()
    helper_uri = "file://" + helper_path
    main_uri = "file://" + main_path
    package_uri = "file://" + package_path
    system_path = os.path.join(
        REPO_ROOT, "packages", "ttx", "Perimortem.System", "terminal.ttx")
    system_package_path = os.path.join(
        REPO_ROOT, "packages", "ttx", "Perimortem.System", "package.ttx")
    with open(system_path, "r", encoding="utf-8") as f:
        system_source = f.read()
    with open(system_package_path, "r", encoding="utf-8") as f:
        system_package_source = f.read()
    system_uri = "file://" + system_path
    system_package_uri = "file://" + system_package_path
    system_package_diagnostics = send_did_open(
        conn, system_package_uri, system_package_source)
    system_diagnostics = send_did_open(conn, system_uri, system_source)
    package_diagnostics = send_did_open(conn, package_uri, package_source)
    helper_diagnostics = send_did_open(conn, helper_uri, helper_source)
    main_diagnostics = send_did_open(conn, main_uri, main_source)
    check(helper_diagnostics is not None and not helper_diagnostics.get(
        "params", {}).get("diagnostics", []),
        "Package helper publishes without diagnostics")
    check(system_diagnostics is not None and not system_diagnostics.get(
        "params", {}).get("diagnostics", []),
        "Perimortem.System source Package publishes without diagnostics")
    check(system_package_diagnostics is not None and
          not system_package_diagnostics.get(
              "params", {}).get("diagnostics", []),
          "Perimortem.System manifest publishes without diagnostics")
    package_messages = (package_diagnostics or {}).get(
        "params", {}).get("diagnostics", [])
    if package_messages:
        print("  Consumer Package diagnostics:")
        for diagnostic in package_messages:
            print("   ", diagnostic.get("message"))
    check(package_diagnostics is not None and not package_diagnostics.get(
        "params", {}).get("diagnostics", []),
        "consumer Package manifest publishes without diagnostics")
    main_messages = (main_diagnostics or {}).get(
        "params", {}).get("diagnostics", [])
    if main_messages:
        print("  Consumer member diagnostics:")
        for diagnostic in main_messages:
            print("   ", diagnostic.get("message"))
    check(main_diagnostics is not None and not main_diagnostics.get(
        "params", {}).get("diagnostics", []),
        "Package member resolves its sibling and Perimortem.System")
    prefix_use = main_source.index("Dynamic::Bytes -> concat")
    system_hover = send_hover(
        conn, main_uri, main_source, "prefix", 11, prefix_use)
    system_result = system_hover.get("result") if system_hover else None
    system_markdown = (
        system_result.get("contents", {}).get("value", "")
        if system_result else "")
    check("state prefix : View[Unsigned_8]" in system_markdown,
          "Package hover uses the shared cross-source analysis snapshot")

    renamed_helper = helper_source.replace("public prefix", "public renamed")
    send_did_change(conn, helper_uri, renamed_helper, 2)
    invalidated_hover = send_hover(
        conn, main_uri, main_source, "prefix", 12, prefix_use)
    check(invalidated_hover is not None and
          invalidated_hover.get("result") is None,
          "editing one member invalidates the complete Package graph")
    send_did_change(conn, helper_uri, helper_source, 3)
    restored_hover = send_hover(
        conn, main_uri, main_source, "prefix", 13, prefix_use)
    restored_result = restored_hover.get("result") if restored_hover else None
    restored_markdown = (
        restored_result.get("contents", {}).get("value", "")
        if restored_result else "")
    check("state prefix : View[Unsigned_8]" in restored_markdown,
          "restoring an overlay rebuilds one complete Package snapshot")

    print("\n--- Semantic tokens: Library/default dialect ---")
    library_source = (
        "dialect : Library;\n"
        "public func run[] -> Count {\n"
        "  pair.left;\n"
        "  pair -> sum();\n"
        "  source -> helper();\n"
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
    library_tokens = semantic_token_texts(library_source, library_data)
    library_texts = [text for text, _ in library_tokens]
    check(len(library_data) > 0, "Library document returns semantic tokens")
    check("while" in library_texts and "continue" in library_texts,
          "Library document highlights loop-control keywords")
    check(("source", 7) in library_tokens,
          "Library document highlights the Source routing keyword")
    check(("left", 5) in library_tokens,
          "address access highlights the selected property")
    check(("sum", 6) in library_tokens,
          "receiver invocation highlights the selected function")

    no_dialect_source = (
        "public func draft[] -> Count {\n"
        "  state label : Text = \"Icon \\\"Preview\\\"\";\n"
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
        "public func main[] -> Count {\n"
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

    print("\n--- Semantic hover: completed Library graph ---")
    hover_path = os.path.join(
        REPO_ROOT, "validation", "data", "ttx", "llvm",
        "runtime.ttx")
    with open(hover_path, "r", encoding="utf-8") as f:
        hover_source = f.read()
    hover_uri = "file:///llvm_nonobject-hover.ttx"
    hover_diagnostics = send_did_open(conn, hover_uri, hover_source)
    check(hover_diagnostics is not None and
          hover_diagnostics.get("method") ==
          "textDocument/publishDiagnostics" and
          not hover_diagnostics.get("params", {}).get("diagnostics", []),
          "valid attributed documentation publishes no diagnostics")
    use_start = hover_source.index("total += OptionOps -> forward")
    present_resp = send_hover(
        conn, hover_uri, hover_source, "present", 30, use_start)
    absent_resp = send_hover(
        conn, hover_uri, hover_source, "absent", 31, use_start)
    present_markdown = (
        present_resp.get("result", {}).get("contents", {}).get("value", "")
        if present_resp else "")
    absent_markdown = (
        absent_resp.get("result", {}).get("contents", {}).get("value", "")
        if absent_resp else "")
    check("const present : Option[Unsigned_64]" in present_markdown,
          "hover resolves present to its exact Field and Type")
    check("= some(5)" in present_markdown,
          "hover displays present's folded Option payload")
    check("const absent : Option[Unsigned_64]" in absent_markdown,
          "hover resolves absent to its exact Field and Type")
    check("= absent" in absent_markdown,
          "hover displays absent's folded Option state")

    frozen_use = hover_source.index("total += frozen_dense")
    frozen_resp = send_hover(
        conn, hover_uri, hover_source, "frozen_dense", 32, frozen_use)
    frozen_markdown = (
        frozen_resp.get("result", {}).get("contents", {}).get("value", "")
        if frozen_resp else "")
    check("const frozen_dense : Fixed[Unsigned_64,4]" in frozen_markdown,
          "hover resolves the const Fixed stack Local and exact Type")
    check("= (5, 6, 7, 8)" in frozen_markdown,
          "hover displays the const Fixed Local's folded values")

    execute_start = hover_source.index("public execute : func")
    dense_use = hover_source.index("total += dense", execute_start)
    dense_resp = send_hover(
        conn, hover_uri, hover_source, "dense", 38, dense_use)
    dense_markdown = (
        (dense_resp.get("result") or {}).get("contents", {}).get("value", "")
        if dense_resp else "")
    check("> Test documentation string for variable" in dense_markdown,
          "Local hover delegates to its Statement documentation")

    function_resp = send_hover(
        conn, hover_uri, hover_source, "execute", 39, execute_start)
    function_markdown = (
        (function_resp.get("result") or {}).get("contents", {}).get("value", "")
        if function_resp else "")
    check("func execute" in function_markdown and
          "> Test documentation string for function" in function_markdown,
          "Function hover includes documentation interleaved with attributes")

    changed_hover_source = hover_source.replace(
        "private const present : Maybe = 5;",
        "private const present : Maybe = 7;")
    send_did_change(conn, hover_uri, changed_hover_source, 2)
    changed_present_resp = send_hover(
        conn, hover_uri, changed_hover_source, "present", 33, use_start)
    changed_present_markdown = (
        changed_present_resp.get("result", {})
        .get("contents", {}).get("value", "")
        if changed_present_resp else "")
    check("= some(7)" in changed_present_markdown,
          "document edits rebuild the semantic hover snapshot")

    detail_source = (
        "// Semantic hover details.\n"
        "dialect : Library;\n"
        "// Storage Type documentation.\n"
        "public Bucket : struct {\n"
        "  // Current value documentation.\n"
        "  public state value : Unsigned_64 = 1;\n"
        "}\n"
        "// Alias documentation.\n"
        "public BucketAlias : alias = Bucket;\n"
        "private inspect : func = [] -> Unsigned_64 {\n"
        "  state bucket : BucketAlias = (.value = 2);\n"
        "  return bucket.value;\n"
        "}\n"
    )
    detail_uri = "file:///semantic-hover-details.ttx"
    send_did_open(conn, detail_uri, detail_source)

    field_start = detail_source.index("public state value")
    field_resp = send_hover(
        conn, detail_uri, detail_source, "value", 34, field_start)
    field_markdown = (
        field_resp.get("result", {}).get("contents", {}).get("value", "")
        if field_resp else "")
    check("state value : Unsigned_64" in field_markdown,
          "hover resolves a state Field declaration and exact Type")
    check("**Kind:** State field" in field_markdown and
          "**Type:** `Unsigned_64`" in field_markdown,
          "state Field hover includes styled semantic details")
    check("**Documentation**" in field_markdown and
          "> Current value documentation." in field_markdown,
          "state Field hover includes attached documentation")

    type_start = detail_source.index("public Bucket : struct")
    type_resp = send_hover(
        conn, detail_uri, detail_source, "Bucket", 35, type_start)
    type_markdown = (
        (type_resp.get("result") or {}).get("contents", {}).get("value", "")
        if type_resp else "")
    check("type Bucket" in type_markdown and
          "**Kind:** Type" in type_markdown,
          "hover resolves an authored Type declaration")
    check("> Storage Type documentation." in type_markdown,
          "Type hover includes attached documentation")

    local_start = detail_source.index("state bucket")
    local_resp = send_hover(
        conn, detail_uri, detail_source, "bucket", 36, local_start)
    local_markdown = (
        (local_resp.get("result") or {}).get("contents", {}).get("value", "")
        if local_resp else "")
    check("state bucket : Bucket" in local_markdown and
          "**Kind:** State local" in local_markdown and
          "**Type:** `Bucket`" in local_markdown,
          "hover resolves a state Local declaration and resolved Type")

    alias_resp = send_hover(
        conn, detail_uri, detail_source, "BucketAlias", 37, local_start)
    alias_markdown = (
        (alias_resp.get("result") or {}).get("contents", {}).get("value", "")
        if alias_resp else "")
    check("BucketAlias : alias = Bucket" in alias_markdown and
          "**Kind:** Type alias" in alias_markdown and
          "**Resolves to:** `Bucket`" in alias_markdown,
          "hover preserves the authored Alias at a Type reference")
    check(alias_markdown.index("Alias documentation.") <
          alias_markdown.index("Storage Type documentation.")
          if "Alias documentation." in alias_markdown and
          "Storage Type documentation." in alias_markdown else False,
          "Alias hover propagates local then target documentation")

    diagnostic_source = (
        "// Invalid hover source.\n"
        "dialect : Library;\n"
        "private broken : func = [] -> [];\n"
    )
    diagnostic_uri = "file:///semantic-diagnostic.ttx"
    diagnostic_resp = send_did_open(
        conn, diagnostic_uri, diagnostic_source)
    diagnostics = (
        diagnostic_resp.get("params", {}).get("diagnostics", [])
        if diagnostic_resp else [])
    check(diagnostic_resp is not None and
          diagnostic_resp.get("method") ==
          "textDocument/publishDiagnostics" and diagnostics and
          "Library Blocks require" in diagnostics[0].get("message", ""),
          "semantic failures publish editor diagnostics")

    long_lines = "".join(
        f"// Hover documentation line {index:03d} carries retained text.\n"
        for index in range(100))
    long_hover_source = (
        "// Long hover source.\n"
        "dialect : Library;\n" + long_lines +
        "public Documented : struct {}\n"
    )
    long_hover_uri = "file:///semantic-long-hover.ttx"
    long_diagnostics = send_did_open(
        conn, long_hover_uri, long_hover_source)
    long_hover_resp = send_hover(
        conn, long_hover_uri, long_hover_source, "Documented", 40)
    long_markdown = (
        (long_hover_resp.get("result") or {})
        .get("contents", {}).get("value", "")
        if long_hover_resp else "")
    check(long_diagnostics is not None and len(long_markdown) > 4096 and
          "Hover documentation line 099" in long_markdown,
          "Arena-backed hover output preserves documentation beyond 4 KiB")

    print("\n--- Ignored notifications ---")
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "method": "$/cancelRequest",
        "params": {"id": 999},
    }))
    notification_probe_resp = send_semantic_tokens(conn, shader_uri, 23)
    check(
        notification_probe_resp is not None
        and "result" in notification_probe_resp,
        "$ notification keeps server responsive")

    print("\n--- Round-trip: apps/ttx/scene_lifetime/scenes/splash.ttx ---")
    splash_path = os.path.join(
        REPO_ROOT, "apps", "ttx", "scene_lifetime", "scenes", "splash.ttx")
    with open(splash_path, "r", encoding="utf-8") as f:
        splash_source = f.read()

    splash_formatted = send_format(conn, splash_source, "splash.ttx")
    if splash_formatted is not None:
        if splash_formatted == splash_source:
            print("  [OK] splash.ttx is unchanged after formatting")
        else:
            print("  [DIFF] splash.ttx changed after formatting:")
            src_lines = splash_source.splitlines()
            fmt_lines = splash_formatted.splitlines()
            for i, (a, b) in enumerate(zip(src_lines, fmt_lines), 1):
                if a != b:
                    print(f"    line {i}:")
                    print(f"      before: {repr(a)}")
                    print(f"      after:  {repr(b)}")
            if len(src_lines) != len(fmt_lines):
                print(f"  line count: {len(src_lines)} → {len(fmt_lines)}")
        check(splash_formatted == splash_source,
              "tracked TTX source is already canonical")
        splash_second = send_format(conn, splash_formatted, "splash.ttx")
        check(splash_second == splash_formatted,
              "document formatting is byte-idempotent")

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
        check("$" in invalid_formatted,
              "formatting preserves malformed authored content")
        check(invalid_formatted.startswith(
            "//\n// Place holder source documentation.\n//\n"),
            "formatting supplies missing source documentation")

    exit_code = proc.poll()
    if exit_code is None:
        print("\nServer still running. Shutting down.")
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
