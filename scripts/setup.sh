#!/usr/bin/env bash
set -euo pipefail

echo "=== AirGuitar Setup ==="

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "Installing system dependencies..."
if ! command -v brew &> /dev/null; then
    echo "Error: Homebrew is required. Install from https://brew.sh"
    exit 1
fi

brew install opencv cmake ninja 2>&1 | tail -5

echo "Creating models directory..."
mkdir -p "$PROJECT_DIR/models"

echo "Downloading MediaPipe TFLite models..."
bash "$SCRIPT_DIR/download_models.sh"

echo ""
echo "=== Setup Complete ==="
echo ""
echo "To build:"
echo "  cmake --preset debug"
echo "  cmake --build build/debug"
echo ""
echo "To run tests:"
echo "  ctest --preset debug"
echo ""
echo "To run the app:"
echo "  open build/debug/AirGuitar.app"
