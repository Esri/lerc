cmake_minimum_required(VERSION 3.12)

project(Lerc
        DESCRIPTION "Limited Error Raster Compression"
        HOMEPAGE_URL "https://github.com/Esri/lerc"
        VERSION 4.0.0) # Keep in sync with Lerc_c_api.h

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCES
    "src/LercLib/*"
    "src/LercLib/Lerc1Decode/*"
    "src/LercLib/include/*"
)

# Make an option, defaulting to shared libs, but allow -DBUILD_SHARED_LIBS=OFF
option (BUILD_SHARED_LIBS "Build shared libraries (set to OFF to build static libs)" ON)

# If no SHARED or STATIC is specified explicitly, add_library will honor BUILD_SHARED_LIBS
add_library(Lerc ${SOURCES})

set_target_properties(Lerc
    PROPERTIES
    PUBLIC_HEADER "src/LercLib/include/Lerc_types.h;src/LercLib/include/Lerc_c_api.h")

if(BUILD_SHARED_LIBS)
    set_target_properties(Lerc
        PROPERTIES
        SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR}
        DEFINE_SYMBOL LERC_EXPORTS)
else()
    set_target_properties(Lerc
        PROPERTIES
        COMPILE_DEFINITIONS LERC_STATIC)
endif()

install(
    TARGETS Lerc
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Handle both absolute paths (e.g. NixOS) and relative for a relocatable package
if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
    set(LERC_PC_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
else()
    set(LERC_PC_INCLUDEDIR "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
endif()
if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
    set(LERC_PC_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
else()
    set(LERC_PC_LIBDIR "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
endif()

# Configure and install pkgconfig file
configure_file(Lerc.pc.in ${CMAKE_CURRENT_BINARY_DIR}/Lerc.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Lerc.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
