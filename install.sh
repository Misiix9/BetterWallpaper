#!/bin/bash

set -e

RESET='\033[0m'
BOLD='\033[1m'
RED='\033[31m'
GREEN='\033[32m'
BLUE='\033[34m'
YELLOW='\033[33m'

log_info() { echo -e "${BLUE}[INFO]${RESET} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${RESET} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${RESET} $1"; }
log_error() { echo -e "${RED}[ERROR]${RESET} $1"; }

# --- 1. Dependency Installation ---

install_dependencies() {
    log_info "Checking dependencies..."
    
    DISTRO=""
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO=$ID
    else
        log_warn "Could not detect distribution via /etc/os-release."
    fi

    check_ubuntu() {
        log_info "Detected Debian/Ubuntu/Pop!_OS family..."
        sudo apt update
        sudo apt install -y build-essential cmake git pkg-config \
            libgtk-4-dev libadwaita-1-dev libgtk-layer-shell-dev \
            libcurl4-openssl-dev libwayland-dev wayland-protocols \
            libmpv-dev libglew-dev \
            nlohmann-json3-dev

        if ! command -v steamcmd &> /dev/null; then
            log_info "Installing SteamCMD..."
            sudo add-apt-repository -y multiverse || true
            sudo dpkg --add-architecture i386 || true
            sudo apt update
            # Pre-accept steam license to avoid interactive hang
            echo steam steam/question select "I AGREE" | sudo debconf-set-selections || true
            echo steam steam/license note '' | sudo debconf-set-selections || true
            sudo apt install -y steamcmd
        else
            log_success "SteamCMD is already installed."
        fi
    }

    check_arch() {
        log_info "Detected Arch Linux family..."
        AUR_HELPER=""
        if command -v yay &> /dev/null; then AUR_HELPER="yay"; 
        elif command -v paru &> /dev/null; then AUR_HELPER="paru"; fi
        
        if [ -z "$AUR_HELPER" ]; then
            log_error "This script requires an AUR helper (yay or paru) for Arch Linux to install dependencies."
            return 1
        fi

        $AUR_HELPER -S --needed base-devel cmake git gtk4 libadwaita gtk4-layer-shell \
            curl wayland wayland-protocols mpv glew nlohmann-json steamcmd
    }

    check_fedora() {
        log_info "Detected Fedora family..."
        sudo dnf groupinstall -y "Development Tools"
        sudo dnf install -y cmake git gcc-c++ \
            gtk4-devel libadwaita-devel gtk4-layer-shell-devel \
            libcurl-devel wayland-devel wayland-protocols-devel \
            mpv-libs-devel glew-devel json-devel

        if ! command -v steamcmd &> /dev/null; then
            log_info "Installing SteamCMD..."
            sudo dnf install -y steamcmd || log_warn "Failed to check/install steamcmd via dnf. Ensure RPM Fusion is enabled."
        fi
    }

    case "$DISTRO" in
        ubuntu|debian|pop|linuxmint)
            check_ubuntu
            ;;
        arch|manjaro|endeavouros)
            check_arch
            ;;
        fedora)
            check_fedora
            ;;
        *)
            log_warn "Unsupported distribution '$DISTRO' for automatic dependency installation."
            log_info "Please ensure the following are installed: gtk4, libadwaita, gtk4-layer-shell, mpv, curl, glew, nlohmann-json, steamcmd, build tools."
            read -p "Press Enter to continue..."
            ;;
    esac
}

# --- 2. Build & Install ---

main_install() {
    install_dependencies

    BIN_DIR="$HOME/.local/bin"
    APP_DIR="$HOME/.local/share/applications"
    ICON_DIR="$HOME/.local/share/icons/hicolor/512x512/apps"
    PIXMAP_DIR="$HOME/.local/share/pixmaps"
    DATA_DIR="$HOME/.local/share/betterwallpaper"

    mkdir -p "$BIN_DIR" "$APP_DIR" "$ICON_DIR" "$PIXMAP_DIR" "$DATA_DIR/ui"

    log_info "Installing resources..."
    if [ -f "data/ui/style.css" ]; then
        cp data/ui/style.css "$DATA_DIR/ui/style.css"
        log_success "Style copied."
    else
        log_warn "data/ui/style.css not found!"
    fi

    log_info "Building project..."
    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release -j$(nproc)

    if [ ! -f "build/src/gui/betterwallpaper" ]; then
        log_error "Build failed: GUI binary not found."
        exit 1
    fi

    log_info "Installing binaries..."
    cp build/src/gui/betterwallpaper "$BIN_DIR/betterwallpaper"
    chmod +x "$BIN_DIR/betterwallpaper"

    if [ -f "build/src/daemon/betterwallpaper-daemon" ]; then
        cp build/src/daemon/betterwallpaper-daemon "$BIN_DIR/betterwallpaper-daemon"
        chmod +x "$BIN_DIR/betterwallpaper-daemon"
        
        # Optional system-wide install for daemon if user wants it
        if [ -w /usr/bin ] || command -v sudo &>/dev/null; then
             log_info "Copying daemon to /usr/bin (sudo may be requested)..."
             sudo cp build/src/daemon/betterwallpaper-daemon /usr/bin/betterwallpaper-daemon || log_warn "Skipped /usr/bin install."
             sudo chmod +x /usr/bin/betterwallpaper-daemon 2>/dev/null || true
        fi
    fi

    if [ -f "betterwallpaper_icon.png" ]; then
        log_info "Installing icon..."
        cp betterwallpaper_icon.png "$ICON_DIR/betterwallpaper.png"
        cp betterwallpaper_icon.png "$PIXMAP_DIR/betterwallpaper.png"
    fi

    log_info "Creating desktop entry..."
    cat > betterwallpaper.desktop <<EOF
[Desktop Entry]
Type=Application
Name=BetterWallpaper
Comment=Manage and set animated wallpapers
Exec=$BIN_DIR/betterwallpaper
Icon=betterwallpaper
Terminal=false
Categories=Utility;GTK;
Keywords=wallpaper;background;animated;live;
StartupNotify=true
EOF

    cp betterwallpaper.desktop "$APP_DIR/"
    
    # Update caches
    gtk-update-icon-cache "$HOME/.local/share/icons/hicolor" 2>/dev/null || true
    update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true

    log_success "Installation complete!"
    echo -e "${BOLD}You can now launch BetterWallpaper from your application menu.${RESET}"
}

main_install
