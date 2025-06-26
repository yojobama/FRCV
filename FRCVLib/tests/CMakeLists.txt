find_package(GTest CONFIG REQUIRED)

add_executable(TestFRCV
    test.cpp
)

target_include_directories(TestFRCV PRIVATE
        FRCVLib
        GTest::gtest_main
)

enable_testing()

