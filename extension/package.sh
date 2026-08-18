#!/bin/bash
# Perimortem Engine
# Copyright © Matt Kaes
#
# Builds Puffer and synchronizes the VSCode extension LSP server. The default
# mode packages a VSIX after synchronization.
# Run from anywhere inside the repository.
#
# Usage:
#   ./extension/package.sh [--sync | --install]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
VSIX_DIR="$REPO_ROOT/.vscode"
SERVER_BIN="$REPO_ROOT/.bin/bin/puffer/puffer"
PACKAGE_SERVER="$SCRIPT_DIR/puffer"

INSTALL=0
SYNC=0
for arg in "$@"; do
  case "$arg" in
    --install) INSTALL=1 ;;
    --sync) SYNC=1 ;;
    *) echo "Unknown argument: $arg" >&2; exit 1 ;;
  esac
done

if [ "$INSTALL" -eq 1 ] && [ "$SYNC" -eq 1 ]; then
  echo "--sync and --install cannot be combined" >&2
  exit 1
fi

echo "==> Building Puffer LSP server (release)..."
cd "$REPO_ROOT"
bazel build --config=release //puffer:puffer

if [ ! -x "$SERVER_BIN" ]; then
  echo "Expected server binary was not created: $SERVER_BIN" >&2
  exit 1
fi

echo "==> Copying latest language server into extension package..."
rm -f "$PACKAGE_SERVER" "$SCRIPT_DIR/ttx-lang-server"
cp -L "$SERVER_BIN" "$PACKAGE_SERVER"
chmod 755 "$PACKAGE_SERVER"

if [ -L "$PACKAGE_SERVER" ]; then
  echo "Packaged server must be a real file, not a symlink: $PACKAGE_SERVER" >&2
  exit 1
fi

echo "==> Installing npm dependencies..."
cd "$SCRIPT_DIR"
npm install --silent

echo "==> Compiling TypeScript..."
npm run compile

if [ "$SYNC" -eq 1 ]; then
  echo "==> Development extension synchronized."
  exit 0
fi

echo "==> Reading extension manifest..."
PACKAGE_NAME="$(node -p "require('$SCRIPT_DIR/package.json').name")"
PACKAGE_PUBLISHER="$(node -p "require('$SCRIPT_DIR/package.json').publisher")"
PACKAGE_VERSION="$(node -p "require('$SCRIPT_DIR/package.json').version")"
VSIX_NAME="${PACKAGE_NAME}-${PACKAGE_VERSION}.vsix"
VSIX="$VSIX_DIR/$VSIX_NAME"

echo "==> Removing an existing output for this version..."
rm -f "$VSIX"

echo "==> Packaging extension..."
npm run package -- --out "$VSIX" --no-rewrite-relative-links

if [ ! -f "$VSIX" ]; then
  echo "Expected VSIX was not created: $VSIX" >&2
  exit 1
fi

echo "==> Packaged: $VSIX"

if [ "$INSTALL" -eq 1 ]; then
  echo "==> Installing extension into VS Code..."
  code --uninstall-extension "$PACKAGE_PUBLISHER.$PACKAGE_NAME" || true
  code --install-extension "$VSIX" --force
  echo "==> Done. Reload VS Code to activate the new version."
fi
