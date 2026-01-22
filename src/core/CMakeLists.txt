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
    ${WAYLAND_CLIENT_LIBRARIES}
    Threads::Threads
    GTK4::gtk4
)

# Add sources
target_sources(bwp_core PRIVATE
    config/ConfigManager.cpp
    config/ProfileManager.cpp
    ipc/DBusService.cpp
    ipc/DBusClient.cpp
    monitor/MonitorManager.cpp
    monitor/WaylandMonitor.cpp
    utils/FileUtils.cpp
    utils/Logger.cpp
    utils/StringUtils.cpp
    wallpaper/WallpaperManager.cpp
    wallpaper/WallpaperWindow.cpp
    wallpaper/renderers/StaticRenderer.cpp
    wallpaper/renderers/VideoRenderer.cpp
    wallpaper/renderers/WallpaperEngineRenderer.cpp
    wallpaper/WallpaperLibrary.cpp
    wallpaper/LibraryScanner.cpp
    wallpaper/TagManager.cpp
    wallpaper/FolderManager.cpp
    
    # Transition
    transition/TransitionEngine.cpp
    transition/effects/BasicEffects.cpp
    
    # Hyprland
    hyprland/HyprlandIPC.cpp
    hyprland/HyprlandManager.cpp
    
    # Steam
    steam/SteamWorkshopClient.cpp
)

# Installation
install(TARGETS bwp_core 
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
