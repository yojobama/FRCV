file(GLOB_RECURSE source_files
        "*.cpp"
        "*.h"
        "Sources/*.cpp"
        "Sources/*.h"
)

add_library(FRCVLib SHARED
    ${source_files}
)

# Ensure all source directories are included
target_include_directories(FRCVLib PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Sources>
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

