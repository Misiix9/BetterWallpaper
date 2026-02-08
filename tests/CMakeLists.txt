# ──────────────────────────────────────────────────────────
#  GoogleTest Integration
# ──────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
# Prevent GoogleTest from overriding our compiler/linker options
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# ──────────────────────────────────────────────────────────
#  Unit tests
# ──────────────────────────────────────────────────────────
add_executable(unit_tests
    unit/SafeProcessTests.cpp
    unit/ConfigManagerTests.cpp
    unit/WallpaperLibraryTests.cpp
    unit/SchedulerTests.cpp
)

target_link_libraries(unit_tests PRIVATE
    bwp_core
    GTest::gtest_main
)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

gtest_discover_tests(unit_tests)
