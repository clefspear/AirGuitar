include(FetchContent)

FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.3
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)

set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_SHADERS OFF CACHE BOOL "" FORCE)

message(STATUS "Fetching JUCE (first build may download source)...")
FetchContent_MakeAvailable(JUCE)
message(STATUS "JUCE fetched at: ${JUCE_SOURCE_DIR}")
