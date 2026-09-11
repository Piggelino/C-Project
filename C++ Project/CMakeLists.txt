cmake_minimum_required(VERSION 3.16)

project(camera_grab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Locate the ARM64 OpenCV installation from the user's vcpkg tree when no
# package directory was supplied by the selected CMake toolchain.
if(NOT OpenCV_DIR)
	set(_user_opencv_dir "$ENV{USERPROFILE}/vcpkg/installed/arm64-windows/share/opencv4")
	if(EXISTS "${_user_opencv_dir}/OpenCVConfig.cmake")
		set(OpenCV_DIR "${_user_opencv_dir}")
	endif()
endif()

set(_user_vcpkg_arm64 "$ENV{USERPROFILE}/vcpkg/installed/arm64-windows")
if(EXISTS "${_user_vcpkg_arm64}")
	list(PREPEND CMAKE_PREFIX_PATH "${_user_vcpkg_arm64}")
	set(Protobuf_DIR "${_user_vcpkg_arm64}/share/protobuf" CACHE PATH "Protobuf package directory")
endif()

find_package(OpenCV CONFIG REQUIRED)

# Build main.cpp as an executable named camera_grab.
add_executable(camera_grab main.cpp)

# Give the program access to OpenCV headers and libraries.
target_include_directories(camera_grab PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(camera_grab PRIVATE ${OpenCV_LIBS})
