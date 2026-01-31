add_library(bwp_core)

target_include_directories(bwp_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Link dependencies
target_link_libraries(bwp_core PUBLIC
    nlohmann_json::nlohmann_json
    ${MPV_LIBRARIES}
    ${CURL_LIBRARIES}
    ${WAYLAND_CLIENT_LIBRARIES}
    ${GTK4_LAYER_SHELL_LIBRARIES}
    Threads::Threads
    GTK4::gtk4
    LIBADWAITA::libadwaita-1
    bwp_ipc
)

# IPC Library (Shared between GTK4 core and GTK3 tray)
add_library(bwp_ipc)
target_sources(bwp_ipc PRIVATE
    ipc/DBusService.cpp
    ipc/DBusClient.cpp
    utils/Logger.cpp # Logger is needed by everyone
)
target_include_directories(bwp_ipc PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(bwp_ipc PUBLIC
    Threads::Threads
    GIO::gio-2.0
)

# Add sources
target_sources(bwp_core PRIVATE
    config/ConfigManager.cpp
    config/ProfileManager.cpp

    monitor/MonitorManager.cpp
    monitor/WaylandMonitor.cpp
    utils/Error.cpp
    utils/FileUtils.cpp
    utils/StringUtils.cpp
    wallpaper/WallpaperManager.cpp
    wallpaper/WallpaperWindow.cpp
    wallpaper/renderers/StaticRenderer.cpp
    wallpaper/renderers/VideoRenderer.cpp
    wallpaper/renderers/WallpaperEngineRenderer.cpp
    
    # Native wallpaper setter
    wallpaper/NativeWallpaperSetter.cpp
    wallpaper/WallpaperLibrary.cpp
    wallpaper/LibraryScanner.cpp
    wallpaper/ThumbnailCache.cpp
    wallpaper/TagManager.cpp
    wallpaper/FolderManager.cpp
    
    # Transition
    transition/TransitionEngine.cpp
    transition/effects/BasicEffects.cpp
    transition/effects/AdvancedEffects.cpp
    
    # Hyprland
    hyprland/HyprlandIPC.cpp
    hyprland/HyprlandManager.cpp
    
    # Steam
    steam/SteamWorkshopClient.cpp
    steam/DownloadQueue.cpp
    
    # Scheduler
    scheduler/Scheduler.cpp
    
    # Slideshow
    slideshow/SlideshowManager.cpp
    
    # Theming
    theming/ColorExtractor.cpp
    theming/ThemeApplier.cpp
    
    # Notifications
    notification/NotificationManager.cpp
    
    # System
    input/InputManager.cpp
    power/PowerManager.cpp
    system/AutostartManager.cpp
    utils/SystemUtils.cpp
)

# Installation
install(TARGETS bwp_core 
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
