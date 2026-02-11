if(WIN32)
    add_executable(betterwallpaper-tray
        main.cpp
    )
    # Link native libs if needed, e.g. user32
    target_link_libraries(betterwallpaper-tray PRIVATE
        bwp_ipc
    )
else()
    add_executable(betterwallpaper-tray
        main.cpp
        LinuxTrayIcon.cpp
        ../core/utils/ProcessUtils.cpp
        ../core/utils/SafeProcess.cpp
    )
    target_link_libraries(betterwallpaper-tray PRIVATE
        bwp_ipc
        nlohmann_json::nlohmann_json
        PkgConfig::GTK3
        PkgConfig::APPINDICATOR3
    )
endif()

install(TARGETS betterwallpaper-tray
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
