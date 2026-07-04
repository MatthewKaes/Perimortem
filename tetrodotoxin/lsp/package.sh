#!/bin/bash
# Perimortem Engine
# Copyright © Matt Kaes
#
# Builds Puffer and packages it as a VSCode extension LSP server (.vsix).
# Run from anywhere inside the repository.
#
# Usage:
#   ./tetrodotoxin/lsp/package.sh [--install]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
VSIX_DIR="$REPO_ROOT/.vscode"
SERVER_BIN="$REPO_ROOT/.bin/bin/tetrodotoxin/puffer"
PACKAGE_SERVER="$SCRIPT_DIR/puffer"

INSTALL=0
for arg in "$@"; do
  case "$arg" in
    --install) INSTALL=1 ;;
    *) echo "Unknown argument: $arg" >&2; exit 1 ;;
  esac
done

echo "==> Reading extension manifest..."
PACKAGE_NAME="$(node -p "require('$SCRIPT_DIR/package.json').name")"
PACKAGE_VERSION="$(node -p "require('$SCRIPT_DIR/package.json').version")"
VSIX_NAME="${PACKAGE_NAME}-${PACKAGE_VERSION}.vsix"
VSIX="$VSIX_DIR/$VSIX_NAME"

echo "==> Building Puffer LSP server (release)..."
cd "$REPO_ROOT"
bazel build --config=release //tetrodotoxin:puffer

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

echo "==> Removing stale VSIX artifacts..."
find "$SCRIPT_DIR" -maxdepth 1 -name "${PACKAGE_NAME}-*.vsix" ! -name "$VSIX_NAME" -delete
find "$VSIX_DIR" -maxdepth 1 -name "${PACKAGE_NAME}-*.vsix" ! -name "$VSIX_NAME" -delete
rm -f "$VSIX"

echo "==> Packaging extension..."
npm run package -- --out "$VSIX" --no-rewrite-relative-links

if [ ! -f "$VSIX" ]; then
  echo "Expected VSIX was not created: $VSIX" >&2
  exit 1
fi

echo "==> Adding VSIX to git index..."
git -C "$REPO_ROOT" add -f "$VSIX"

echo "==> Packaged: $VSIX"

if [ "$INSTALL" -eq 1 ]; then
  echo "==> Installing extension into VS Code..."
  code --install-extension "$VSIX"
  echo "==> Done. Reload VS Code to activate the new version."
fi
