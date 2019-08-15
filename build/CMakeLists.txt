# Minimum version, considered by the authors of CMake, to
# support modern functionality provided by the framework
cmake_minimum_required(VERSION 3.2)

project("particle_device_os")

# Set built-in CMake toolchain variables
# NOTE: Keep this in sync with lang-std.mk
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_C_STANDARD 11)

if("${PARTICLE_PLATFORM_ID}" EQUAL "3")
  message(TRACE "Platform 3")
  include("platformGcc.cmake")
else()
  message(TRACE "Not platform 3")
endif()
