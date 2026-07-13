set(LIB_SOURCES
  App/Application.cpp
  App/MainComponent.cpp
  Vision/Camera.cpp
  Vision/TFLiteRuntime.cpp
  Vision/PalmDetector.cpp
  Vision/HandLandmarker.cpp
  Vision/PoseLandmarker.cpp
  Vision/HandPipeline.cpp
  Physics/OneEuroFilter.cpp
  Physics/FretboardTracker.cpp
  Physics/StrumDetector.cpp
  Physics/ChordClassifier.cpp
  Physics/PhysicsEngine.cpp
  Calibration/CalibrationManager.cpp
  Audio/KarplusStrong.cpp
  Audio/StringModel.cpp
  Audio/AudioEngine.cpp
  Audio/MidiOutput.cpp
)

set(LIB_HEADERS
  App/Application.h
  App/MainComponent.h
  App/CrashLogger.h
  Vision/LandmarkData.h
  Vision/Camera.h
  Vision/TFLiteRuntime.h
  Vision/PalmDetector.h
  Vision/HandLandmarker.h
  Vision/PoseLandmarker.h
  Vision/HandPipeline.h
  Physics/OneEuroFilter.h
  Physics/NoteEvent.h
  Physics/CalibrationData.h
  Physics/PhysicsData.h
  Physics/FretboardTracker.h
  Physics/StrumDetector.h
  Physics/ChordClassifier.h
  Physics/PhysicsEngine.h
  Calibration/CalibrationManager.h
  Audio/KarplusStrong.h
  Audio/StringModel.h
  Audio/AudioEngine.h
  Audio/MidiOutput.h
)

add_library(AirGuitarLib STATIC
  ${LIB_SOURCES}
  ${LIB_HEADERS}
)

target_include_directories(AirGuitarLib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${OpenCV_INCLUDE_DIRS}
  ${TENSORFLOW_SOURCE_DIR}
)

target_link_libraries(AirGuitarLib PUBLIC
  juce::juce_audio_basics
  juce::juce_audio_devices
  juce::juce_audio_processors
  juce::juce_audio_utils
  juce::juce_gui_basics
  juce::juce_graphics
  juce::juce_opengl
  juce::juce_core
  ${OpenCV_LIBS}
  tensorflow-lite
)

target_compile_definitions(AirGuitarLib PUBLIC
  JUCE_MODAL_LOOPS_PERMITTED=1
  JUCE_USE_CURL=0
  $<$<CONFIG:Debug>:JUCE_DEBUG=1>
  $<$<CONFIG:Debug>:AIRGUITAR_DEBUG_FRAME_DUMP=1>
)

add_executable(AirGuitar
  main.cpp
)

target_link_libraries(AirGuitar PRIVATE
  AirGuitarLib
)

set_project_warnings(AirGuitar)

if(APPLE)
  set_target_properties(AirGuitar PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.airguitar.app"
    MACOSX_BUNDLE_BUNDLE_NAME "AirGuitar"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES"
  )

  set(MODELS_DIR "${CMAKE_SOURCE_DIR}/models")
  file(GLOB MODEL_FILES "${MODELS_DIR}/*.tflite")

  if(MODEL_FILES)
    foreach(MODEL_FILE ${MODEL_FILES})
      get_filename_component(MODEL_NAME ${MODEL_FILE} NAME)
      set_source_files_properties(${MODEL_FILE} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
      )
      target_sources(AirGuitar PRIVATE ${MODEL_FILE})
    endforeach()

    set_target_properties(AirGuitar PROPERTIES
      RESOURCE "${MODEL_FILES}"
    )
  endif()
endif()
