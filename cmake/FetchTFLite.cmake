include(FetchContent)

FetchContent_Declare(
  tensorflow
  GIT_REPOSITORY https://github.com/tensorflow/tensorflow.git
  GIT_TAG v2.16.1
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)

set(TFLITE_ENABLE_XNNPACK OFF CACHE BOOL "Enable XNNPACK" FORCE)
set(TFLITE_ENABLE_GPU OFF CACHE BOOL "" FORCE)
set(TFLITE_ENABLE_RPC OFF CACHE BOOL "" FORCE)
set(TFLITE_ENABLE_ARMNN OFF CACHE BOOL "" FORCE)
set(TFLITE_ENABLE_MMAP OFF CACHE BOOL "" FORCE)
set(TFLITE_ENABLE_NNAPI OFF CACHE BOOL "" FORCE)
set(TFLITE_ENABLE_HEXAGON OFF CACHE BOOL "" FORCE)
set(TFLITE_ENABLE_RUY OFF CACHE BOOL "Enable RUY matrix multiply" FORCE)

set(TFLITE_ENABLE_TFLITEINSTRUMENTATION OFF CACHE BOOL "" FORCE)

# Force ARM64 system processor so RUY doesn't add unsupported x86 compile flags
# (CMAKE_SYSTEM_PROCESSOR is reported as x86_64 under Rosetta on Apple Silicon)
set(CMAKE_PROJECT_ruy_INCLUDE
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ruy-project-include.cmake"
  CACHE INTERNAL "Hook to override CMAKE_SYSTEM_PROCESSOR for RUY"
)
set(TFLITE_ENABLE_METAL OFF CACHE BOOL "Enable Metal GPU delegate on Apple Silicon" FORCE)

FetchContent_GetProperties(tensorflow)
if(NOT tensorflow_POPULATED)
  message(STATUS "Fetching TensorFlow Lite (first build will download source and may take a while)...")
  FetchContent_Populate(tensorflow)
  message(STATUS "TensorFlow Lite source at: ${tensorflow_SOURCE_DIR}")

  # Patch for Xcode 16+ Clang: std::abs<float> ambiguity with C++17
  set(_elem_file "${tensorflow_SOURCE_DIR}/tensorflow/lite/kernels/elementwise.cc")
  if(EXISTS "${_elem_file}")
    file(READ "${_elem_file}" _elem_content)
    string(REPLACE
      "std::abs<float>"
      "[](float x) { return std::abs(x); }"
      _elem_content "${_elem_content}")
    string(REPLACE
      "std::abs<int32_t>"
      "[](int32_t x) { return std::abs(x); }"
      _elem_content "${_elem_content}")
    file(WRITE "${_elem_file}" "${_elem_content}")
    message(STATUS "Patched elementwise.cc for Xcode 16+ Clang compatibility")
  endif()
endif()

add_subdirectory(
  ${tensorflow_SOURCE_DIR}/tensorflow/lite
  ${tensorflow_BINARY_DIR}/tflite
  EXCLUDE_FROM_ALL
)

message(STATUS "TensorFlow Lite available (target: tensorflow-lite)")
