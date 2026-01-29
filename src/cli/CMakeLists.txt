add_executable(bwp main.cpp)

target_link_libraries(bwp PRIVATE
    bwp_ipc
)

install(TARGETS bwp DESTINATION ${CMAKE_INSTALL_BINDIR})
