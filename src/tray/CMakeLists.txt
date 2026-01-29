add_executable(betterwallpaper-tray
    main.cpp
    TrayIcon.cpp
)

target_link_libraries(betterwallpaper-tray PRIVATE
    bwp_ipc
    PkgConfig::GTK3
    PkgConfig::APPINDICATOR3
)

install(TARGETS betterwallpaper-tray
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
