cmake_minimum_required (VERSION 3.11)
project (Lerc)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCES
    "src/LercLib/*"
    "src/LercLib/Lerc1Decode/*"
    "src/LercLib/include/*"
)

add_library(Lerc SHARED ${SOURCES})
set_target_properties(Lerc PROPERTIES DEFINE_SYMBOL LERC_EXPORTS)

install(
    TARGETS Lerc
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    FILES "src/LercLib/include/Lerc_types.h" "src/LercLib/include/Lerc_c_api.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
