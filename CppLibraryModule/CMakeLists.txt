cmake_minimum_required(VERSION 3.31)
project(CppLibraryModule)

set(CMAKE_CXX_STANDARD 20)

add_library(CppLibraryModule SHARED library.cpp)

find_package(OpenCV)

include(FetchContent)

FetchContent_Declare(
        apriltag
        GIT_REPOSITORY https://github.com/AprilRobotics/apriltag.git
        GIT_TAG master
)

FetchContent_MakeAvailable(apriltag)

target_link_libraries(CppLibraryModule PRIVATE apriltag ${OpenCV_LIBS})