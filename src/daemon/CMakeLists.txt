add_executable(betterwallpaper-daemon main.cpp)

target_link_libraries(betterwallpaper-daemon PRIVATE
    bwp_core
    bwp_ipc
)

install(TARGETS betterwallpaper-daemon DESTINATION ${CMAKE_INSTALL_BINDIR})
