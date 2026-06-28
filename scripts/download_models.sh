#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="${OUT_DIR:-"$SCRIPT_DIR/../models"}"
mkdir -p "$OUT_DIR"

BASE_URL="https://storage.googleapis.com/mediapipe-assets"

# Model download URLs (model_name:url)
# Using arrays instead of associative arrays for bash 3 compatibility
MODEL_NAMES=(
    "palm_detection_lite.tflite"
    "hand_landmark_lite.tflite"
    "pose_landmark_lite.tflite"
)

MODEL_URLS=(
    "${BASE_URL}/palm_detection_lite.tflite"
    "${BASE_URL}/hand_landmark_lite.tflite"
    "${BASE_URL}/pose_landmark_lite.tflite"
)

echo "Downloading models to: $OUT_DIR"

for i in "${!MODEL_NAMES[@]}"; do
    MODEL_NAME="${MODEL_NAMES[$i]}"
    URL="${MODEL_URLS[$i]}"
    TARGET="${OUT_DIR}/${MODEL_NAME}"

    if [ -f "$TARGET" ]; then
        echo "  [SKIP] $MODEL_NAME already exists"
        continue
    fi

    echo "  [FETCH] $MODEL_NAME ..."

    HTTP_CODE=$(curl -s -o "$TARGET" -w "%{http_code}" --fail "$URL" 2>/dev/null || echo "000")

    if [ "$HTTP_CODE" != "200" ]; then
        rm -f "$TARGET"
        echo "  [FAIL] $MODEL_NAME (HTTP $HTTP_CODE)"
        echo "         URL: $URL"
        continue
    fi

    FILE_SIZE=$(stat -f%z "$TARGET" 2>/dev/null || stat -c%s "$TARGET" 2>/dev/null || echo "0")
    if [ "$FILE_SIZE" -lt 1000 ]; then
        rm -f "$TARGET"
        echo "  [FAIL] $MODEL_NAME (file too small: $FILE_SIZE bytes)"
        continue
    fi

    echo "  [OK]   $MODEL_NAME ($(echo "scale=1; $FILE_SIZE/1048576" | bc) MB)"
done

echo ""
echo "Download complete."
ls -lh "$OUT_DIR"/*.tflite 2>/dev/null || echo "No .tflite files found."
