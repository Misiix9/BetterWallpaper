if(WIN32)
    add_executable(betterwallpaper WIN32
        WindowsMain.cpp
        windows/WindowsMainWindow.cpp
        ../core/utils/Logger.cpp
    )
    target_link_libraries(betterwallpaper PRIVATE
        bwp_core
        user32
        gdi32
    )
    install(TARGETS betterwallpaper DESTINATION ${CMAKE_INSTALL_BINDIR})
    return()
endif()

# Linux / GTK4 Build
add_executable(betterwallpaper 
    main.cpp
    Application.cpp
    MainWindow.cpp
    widgets/Sidebar.cpp
    widgets/WallpaperCard.cpp
    widgets/SearchBar.cpp
    widgets/WallpaperGrid.cpp
    widgets/MonitorLayout.cpp
    widgets/MonitorConfigPanel.cpp
    widgets/PreviewPanel.cpp
    widgets/TagEditorDialog.cpp
    widgets/TransitionDialog.cpp
    widgets/ProfileEditDialog.cpp
    widgets/ScheduleDialog.cpp
    widgets/HyprlandWorkspacesView.cpp
    models/WallpaperObject.cpp
    views/LibraryView.cpp
    views/MonitorsView.cpp
    views/FolderView.cpp
    views/WorkshopView.cpp
    views/SettingsView.cpp
    views/FavoritesView.cpp

    views/ProfilesView.cpp
    views/ScheduleView.cpp
    dialogs/SetupDialog.cpp
    dialogs/WorkspaceSelectionDialog.cpp
    dialogs/LibrarySelectionDialog.cpp
)

target_link_libraries(betterwallpaper PRIVATE
    bwp_core
    GTK4::gtk4
    LIBADWAITA::libadwaita-1
)

install(TARGETS betterwallpaper DESTINATION ${CMAKE_INSTALL_BINDIR})
