cmake_minimum_required (VERSION 3.11)
project (Lerc)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCES 
    "src/LercLib/*"
    "src/LercLib/Lerc1Decode/*"
    "include"
)
message(${SOURCES})

add_library(LercLib SHARED ${SOURCES})


target_link_libraries (LercLib)
