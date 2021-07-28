cmake_minimum_required (VERSION 3.11)
project (Lerc)

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
    set_target_properties(Lerc PROPERTIES DEFINE_SYMBOL LERC_EXPORTS)
endif()

install(
    TARGETS Lerc
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
