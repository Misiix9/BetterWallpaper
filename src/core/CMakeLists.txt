add_library(bwp_core)

target_include_directories(bwp_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Link dependencies
if(WIN32)
    target_link_libraries(bwp_core PUBLIC
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
else()
    target_link_libraries(bwp_core PUBLIC
        nlohmann_json::nlohmann_json
        ${MPV_LIBRARIES}
        ${CURL_LIBRARIES}
        ${GTK4_LAYER_SHELL_LIBRARIES}
        ${WAYLAND_CLIENT_LIBRARIES}
        Threads::Threads
        GTK4::gtk4
        LIBADWAITA::libadwaita-1
        bwp_ipc
    )
endif()

# IPC Library (Shared between GTK4 core and GTK3 tray)
add_library(bwp_ipc)

if(WIN32)
    target_sources(bwp_ipc PRIVATE
        utils/Logger.cpp
        ipc/NamedPipeService.cpp 
        ipc/NamedPipeClient.hpp # Headers usually not needed in sources but helpful for IDEs
    )
else()
    target_sources(bwp_ipc PRIVATE
        ipc/DBusService.cpp
        ipc/LinuxIPCClient.cpp
        utils/Logger.cpp
    )
endif()
target_include_directories(bwp_ipc PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

if(WIN32)
    target_link_libraries(bwp_ipc PUBLIC
        Threads::Threads
    )
else()
    target_link_libraries(bwp_ipc PUBLIC
        Threads::Threads
        GIO::gio-2.0
    )
endif()

# Add sources
# Add sources
if(WIN32)
    target_sources(bwp_core PRIVATE
        config/ConfigManager.cpp
        config/ProfileManager.cpp
        monitor/MonitorManager.cpp
        # monitor/WaylandMonitor.cpp # Exclude Wayland
        utils/Error.cpp
        utils/FileUtils.cpp
        utils/StringUtils.cpp
        wallpaper/WallpaperManager.cpp
        
        # Windows specifics (or stub)
        wallpaper/NativeWallpaperSetter.cpp # Has Windows impl
        wallpaper/WallpaperLibrary.cpp
        wallpaper/LibraryScanner.cpp
        wallpaper/ThumbnailCache.cpp
        wallpaper/TagManager.cpp
        wallpaper/FolderManager.cpp

        # Core logic
        scheduler/Scheduler.cpp
        slideshow/SlideshowManager.cpp
        notification/NotificationManager.cpp
        
        # Stub or portable
        # input/InputManager.cpp # Might use Linux specific input?
        
        utils/SystemUtils.cpp
    )
else()
    target_sources(bwp_core PRIVATE
        config/ConfigManager.cpp
        config/ProfileManager.cpp
        monitor/MonitorManager.cpp
        monitor/WaylandMonitor.cpp
        utils/Error.cpp
        utils/FileUtils.cpp
        utils/StringUtils.cpp
        utils/ProcessUtils.cpp
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
        wallpaper/WallpaperTransitionManager.cpp
        wallpaper/WallpaperPreloader.cpp
        
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
        input/KeybindManager.cpp
        power/PowerManager.cpp
        system/AutostartManager.cpp
        utils/SystemUtils.cpp
    )
endif()

# Installation
install(TARGETS bwp_core 
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
