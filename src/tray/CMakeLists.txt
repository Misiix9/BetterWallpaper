add_executable(betterwallpaper-tray main.cpp)

target_link_libraries(betterwallpaper-tray PRIVATE
    bwp_core
    GTK4::gtk4
    LIBADWAITA::libadwaita-1
)

install(TARGETS betterwallpaper-tray DESTINATION ${CMAKE_INSTALL_BINDIR})
