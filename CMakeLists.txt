cmake_minimum_required(VERSION 3.5)

project(eigenbus LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(cybootloaderutils)

#Bootloader Dependencies
add_definitions(-DEIGEN_BTLDR_SUPPORT)
set(BTLDR_SOURCES
    cybootloaderutils/cybtldr_api.cpp
    cybootloaderutils/cybtldr_api2.cpp
    cybootloaderutils/cybtldr_command.cpp
    cybootloaderutils/cybtldr_parse.cpp)

add_executable(eigenbus eigen_comms.cpp ${BTLDR_SOURCES})
