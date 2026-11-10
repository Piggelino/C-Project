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