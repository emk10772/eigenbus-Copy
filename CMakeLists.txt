cmake_minimum_required(VERSION 3.5)

project(eigenbus LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(cybootloaderutils)
include_directories(eigen_comms)
include_directories(eigen_responses)

#Bootloader Dependencies
add_definitions(-DEIGEN_BTLDR_SUPPORT)
set(BTLDR_SOURCES
    eigen_bootloader.cpp
    cybootloaderutils/cybtldr_api.cpp
    cybootloaderutils/cybtldr_api2.cpp
    cybootloaderutils/cybtldr_command.cpp
    cybootloaderutils/cybtldr_parse.cpp)

set(MAIN_SOURCES
    eigen_comms.cpp
    eigen_module.cpp
    eigen_packet_filter.cpp
    eigen_packet_parser.cpp
    eigen_utils.cpp
)

add_executable(eigenbus ${MAIN_SOURCES} ${BTLDR_SOURCES})
