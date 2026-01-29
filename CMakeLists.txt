cmake_minimum_required(VERSION 3.20)

project(BetterWallpaper
    VERSION 1.0.0
    DESCRIPTION "Feature-rich wallpaper manager for Linux"
    LANGUAGES CXX C
)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Position Independent Code
# Enable Position Independent Code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add cmake module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET GLOBAL gio-2.0)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET GLOBAL gtk4)
pkg_check_modules(LIBADWAITA REQUIRED IMPORTED_TARGET GLOBAL libadwaita-1)
pkg_check_modules(GTK4_LAYER_SHELL REQUIRED IMPORTED_TARGET GLOBAL gtk4-layer-shell-0)
pkg_check_modules(MPV REQUIRED mpv)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(GTK3 IMPORTED_TARGET GLOBAL gtk+-3.0)
pkg_check_modules(APPINDICATOR3 IMPORTED_TARGET GLOBAL appindicator3-0.1)

# Create aliases for compatibility with existing code
if(TARGET PkgConfig::GTK4)
    add_library(GTK4::gtk4 ALIAS PkgConfig::GTK4)
endif()

if(TARGET PkgConfig::LIBADWAITA)
    add_library(LIBADWAITA::libadwaita-1 ALIAS PkgConfig::LIBADWAITA)
endif()

if(TARGET PkgConfig::GIO)
    add_library(GIO::gio-2.0 ALIAS PkgConfig::GIO)
endif()

find_package(WaylandProtocols REQUIRED)
# Fetch nlohmann_json
include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)
find_package(Threads REQUIRED)

# Compiler warnings
if(MSVC)
    add_compile_options(/W3 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Testing
enable_testing()

# Subdirectories
add_subdirectory(src/core)
add_subdirectory(src/daemon)
add_subdirectory(src/gui)
add_subdirectory(src/cli)
add_subdirectory(src/tray)
add_subdirectory(tests)

# Packaging/Installation
include(GNUInstallDirs)

install(FILES betterwallpaper.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

install(FILES packaging/betterwallpaper-daemon.service
    DESTINATION lib/systemd/user
)

install(FILES packaging/betterwallpaper-tray.desktop
    DESTINATION /etc/xdg/autostart
)
