# FindWaylandProtocols
# ------------------
#
# Finds the wayland-protocols package
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module will set the following variables in your project:
#
# ``WaylandProtocols_FOUND``
#   True if wayland-protocols is found.
# ``WaylandProtocols_VERSION``
#   The version of wayland-protocols found.
# ``WaylandProtocols_DATADIR``
#   The wayland-protocols data directory.

find_package(PkgConfig QUIET)
pkg_check_modules(PC_WAYLAND_PROTOCOLS QUIET wayland-protocols)

if(PC_WAYLAND_PROTOCOLS_FOUND)
    set(WaylandProtocols_VERSION ${PC_WAYLAND_PROTOCOLS_VERSION})
    pkg_get_variable(WaylandProtocols_DATADIR wayland-protocols pkgdatadir)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WaylandProtocols
    FOUND_VAR WaylandProtocols_FOUND
    REQUIRED_VARS WaylandProtocols_DATADIR
    VERSION_VAR WaylandProtocols_VERSION
)

if(WaylandProtocols_FOUND)
    message(STATUS "Found Wayland Protocols: ${WaylandProtocols_DATADIR} (version ${WaylandProtocols_VERSION})")
endif()
