#!/bin/bash
set -e

echo "Detected Arch Linux environment."

# Helper detection
if command -v yay &> /dev/null; then
    HELPER="yay"
elif command -v paru &> /dev/null; then
    HELPER="paru"
else
    echo "Error: Neither yay nor paru found. Please install an AUR helper."
    exit 1
fi

echo "Using AUR helper: $HELPER"

DEPENDENCIES=(
    "gtk4"
    "libadwaita"
    "gtk4-layer-shell"
    "mpv"
    "curl"
    "glew"
    "linux-wallpaperengine"
    "cmake"
    "base-devel"
)

echo "Installing dependencies..."
$HELPER -S --needed --noconfirm "${DEPENDENCIES[@]}"

# Fix for GLEW version mismatch if necessary (creating symlink in local shim dir)
# Only do this if linux-wallpaperengine fails to find the right lib
GLEW_LIB_PATH=$(ldconfig -p | grep libGLEW.so.2.3 | head -n 1 | awk '{print $4}')
if [ -z "$GLEW_LIB_PATH" ]; then
    GLEW_LIB_PATH="/usr/lib/libGLEW.so"
fi

# Shim directory setup just in case
mkdir -p "$HOME/.local/lib_shims"
if [ ! -f "$HOME/.local/lib_shims/libGLEW.so.2.2" ]; then
    echo "Creating compatibility shim for libGLEW.so.2.2..."
    ln -sf "$GLEW_LIB_PATH" "$HOME/.local/lib_shims/libGLEW.so.2.2"
fi

echo "Detailed setup:"
echo "1. Run: export LD_LIBRARY_PATH=\$HOME/.local/lib_shims:\$LD_LIBRARY_PATH"
echo "   (Add this to your shell profile if linux-wallpaperengine complains about GLEW)"

echo "Done! Dependencies installed."
