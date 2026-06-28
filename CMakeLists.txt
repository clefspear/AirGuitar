cmake_minimum_required(VERSION 3.24)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()
project(AirGuitar
  VERSION 0.1.0
  DESCRIPTION "Desktop air guitar using computer vision"
  LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CompilerWarnings)
include(FetchJUCE)
include(FetchTFLite)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio)
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include: ${OpenCV_INCLUDE_DIRS}")

set(TENSORFLOW_SOURCE_DIR "${tensorflow_SOURCE_DIR}")
add_subdirectory(src)

option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
