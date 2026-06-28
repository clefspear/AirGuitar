include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.6.0
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(Catch2)

add_executable(AirGuitarTests
  VisionTests.cpp
  PhysicsTests.cpp
)

target_link_libraries(AirGuitarTests PRIVATE
  Catch2::Catch2WithMain
  AirGuitarLib
)

set_project_warnings(AirGuitarTests)

add_test(NAME AirGuitarTests
         COMMAND AirGuitarTests
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
