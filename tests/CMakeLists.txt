add_executable(unit_tests unit_tests.cpp)

target_link_libraries(unit_tests PRIVATE
    bwp_core
)

add_test(NAME unit_tests COMMAND unit_tests)
