#!/bin/bash
# Perimortem Engine
# Copyright © Matt Kaes
#
# Builds the TTX language server and packages it as a VSCode extension (.vsix).
# Run from anywhere inside the repository.
#
# Usage:
#   ./tetrodotoxin/lsp/package.sh --install

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
CLIENT_DIR="$SCRIPT_DIR/client"

INSTALL=0
for arg in "$@"; do
  case "$arg" in
    --install) INSTALL=1 ;;
    *) echo "Unknown argument: $arg" >&2; exit 1 ;;
  esac
done

echo "==> Building TTX language server (release)..."
cd "$REPO_ROOT"
bazel build --config=release //tetrodotoxin/lsp/server:ttx-lang-server

echo "==> Installing npm dependencies..."
cd "$CLIENT_DIR"
npm install --silent

echo "==> Compiling TypeScript..."
npm run compile

echo "==> Packaging extension..."
npm run package

VSIX=$(ls -t "$CLIENT_DIR"/tetrodotoxin-lsp-*.vsix | head -1)
echo "==> Packaged: $VSIX"

if [ "$INSTALL" -eq 1 ]; then
  echo "==> Installing extension into VS Code..."
  code --install-extension "$VSIX"
  echo "==> Done. Reload VS Code to activate the new version."
fi
