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
)

target_link_libraries(betterwallpaper PRIVATE
    bwp_core
    GTK4::gtk4
    LIBADWAITA::libadwaita-1
)

install(TARGETS betterwallpaper DESTINATION ${CMAKE_INSTALL_BINDIR})
