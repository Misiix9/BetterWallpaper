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
    dialogs/TagDialog.cpp
    views/LibraryView.cpp
    views/MonitorsView.cpp
    views/FolderView.cpp
)

target_link_libraries(betterwallpaper PRIVATE
    bwp_core
    GTK4::gtk4
    LIBADWAITA::libadwaita-1
)

install(TARGETS betterwallpaper DESTINATION ${CMAKE_INSTALL_BINDIR})
