#!/bin/bash
set -e

echo "Detected Pop!_OS / Debian-based environment."

# Update package lists
echo "Updating package lists..."
sudo apt update

# Install basic build tools and dependencies
DEPENDENCIES=(
    "build-essential"
    "cmake"
    "pkg-config"
    "libglib2.0-dev"
    "libcurl4-openssl-dev"
    "libgtk-4-dev"
    "libadwaita-1-dev"
    "libmpv-dev"
    "libwayland-dev"
    "libgtk-3-dev"
    "libayatana-appindicator3-dev"
    "wayland-protocols"
    "git"
    "ninja-build"
)

echo "Installing base dependencies..."
sudo apt install -y "${DEPENDENCIES[@]}"

# Check for gtk4-layer-shell
echo "Checking for gtk4-layer-shell..."
if pkg-config --exists gtk4-layer-shell-0; then
    echo "gtk4-layer-shell found."
else
    echo "gtk4-layer-shell NOT found. Checking if available in apt..."
    if apt-cache show libgtk4-layer-shell-dev &> /dev/null; then
         echo "Installing libgtk4-layer-shell-dev from apt..."
         sudo apt install -y libgtk4-layer-shell-dev
    else
        echo "libgtk4-layer-shell-dev not found in apt. Building from source..."
        
        # Build deps for gtk4-layer-shell
        sudo apt install -y libgirepository1.0-dev valac
        
        WORKDIR=$(mktemp -d)
        cd "$WORKDIR"
        echo "Cloning gtk4-layer-shell..."
        git clone https://github.com/wmww/gtk4-layer-shell.git
        cd gtk4-layer-shell
        
        echo "Building gtk4-layer-shell..."
        meson setup build
        ninja -C build
        sudo ninja -C build install
        sudo ldconfig
        
        cd ..
        rm -rf "$WORKDIR"
        echo "gtk4-layer-shell installed from source."
    fi
fi

echo "Dependencies installed successfully!"
