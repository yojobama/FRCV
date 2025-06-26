add_library(FRCVLib SHARED
        FRCVCore.h
        FRCVCore.cpp
)

target_include_directories(FRCVLib PUBLIC
    $<INSTALL_INTERFACE:include> # For installed library
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> # For local build
)

find_package(OpenCV REQUIRED)
find_package(apriltag REQUIRED)

target_link_libraries(FRCVLib PUBLIC
    ${OpenCV_LIBS}
    apriltag
)