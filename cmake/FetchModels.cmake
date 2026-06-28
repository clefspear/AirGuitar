set(MODELS_DIR "${CMAKE_SOURCE_DIR}/models")
set(MODELS
  "palm_detection_lite.tflite"
  "hand_landmark_lite.tflite"
  "pose_landmark_lite.tflite"
)

set(MODELS_BASE_URL "https://storage.googleapis.com/mediapipe-models")

add_custom_target(download_models
  COMMENT "Downloading MediaPipe TFLite models..."
  ${CMAKE_COMMAND} -E make_directory ${MODELS_DIR}
)

foreach(model ${MODELS})
  add_custom_command(
    TARGET download_models
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E env
      MODEL_NAME=${model}
      OUT_DIR=${MODELS_DIR}
      ${CMAKE_SOURCE_DIR}/scripts/download_models.sh
    COMMENT "Downloading ${model}..."
  )
endforeach()
