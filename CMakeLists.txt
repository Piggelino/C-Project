cmake_minimum_required(VERSION 3.16)

project(camera_grab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenCV can be supplied by vcpkg, Homebrew, or another CMake package
# installation. The toolchain or CMAKE_PREFIX_PATH selects the architecture.
find_package(OpenCV CONFIG REQUIRED
    HINTS
        /opt/homebrew/opt/opencv/lib/cmake/opencv4
        /usr/local/opt/opencv/lib/cmake/opencv4
)

# Build all project source files.
add_executable(camera_grab
    src/main.cpp
    src/stuff/CameraHandler.cpp
    src/stuff/CornerDetector.cpp
    src/audio/AudioSource.cpp
    src/audio/AudioOutput.cpp
    src/stuff/DistanceEstimator.cpp
    src/stuff/MarkerTemplateMatcher.cpp
    src/stuff/RotationEstimator.cpp
)

# Give the program access to OpenCV headers and libraries.
target_include_directories(camera_grab PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    src/audio
    src/stuff
)
target_link_libraries(camera_grab PRIVATE ${OpenCV_LIBS})

if(WIN32)
    target_link_libraries(camera_grab PRIVATE winmm)
endif()

add_custom_command(TARGET camera_grab POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:camera_grab>/resources"
)

# Component tests for DistanceEstimator
add_executable(distance_estimator_test
    tests/DistanceEstimatorTest.cpp
    src/stuff/DistanceEstimator.cpp
)

target_include_directories(distance_estimator_test PRIVATE
    src/stuff
)

# Component tests for CornerDetector
add_executable(corner_detector_test
    tests/CornerDetectorTest.cpp
    src/stuff/CornerDetector.cpp
)

target_include_directories(corner_detector_test PRIVATE
    src/stuff
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(corner_detector_test PRIVATE ${OpenCV_LIBS})


# Component tests for MarkerTemplateMatcher
add_executable(marker_template_matcher_test
    tests/MarkerTemplateMatcherTest.cpp
    src/stuff/MarkerTemplateMatcher.cpp
)

target_include_directories(marker_template_matcher_test PRIVATE
    src/stuff
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(marker_template_matcher_test PRIVATE ${OpenCV_LIBS})


# Component tests for AudioSource
add_executable(audio_source_test
    tests/AudioSourceTest.cpp
    src/audio/AudioSource.cpp
)

target_include_directories(audio_source_test PRIVATE
    src/audio
)


# Component tests for AudioOutput
add_executable(audio_output_test
    tests/AudioOutputTest.cpp
    src/audio/AudioSource.cpp
    src/audio/AudioOutput.cpp
)

target_include_directories(audio_output_test PRIVATE
    src/audio
)

if(WIN32)
    target_link_libraries(audio_output_test PRIVATE winmm)
endif()