#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>

#include "Vision/LandmarkData.h"
#include "Vision/TFLiteRuntime.h"
#include "Vision/Camera.h"
#include "Vision/PalmDetector.h"
#include "Vision/HandLandmarker.h"
#include "Vision/PoseLandmarker.h"

using namespace AirGuitar;
using Catch::Approx;

// ── LandmarkData Tests ────────────────────────────────────────────────

TEST_CASE("Landmark defaults to zero", "[LandmarkData]")
{
    Landmark lm;
    REQUIRE(lm.x == Approx(0.0f));
    REQUIRE(lm.y == Approx(0.0f));
    REQUIRE(lm.z == Approx(0.0f));
    REQUIRE(lm.visibility == Approx(0.0f));
    REQUIRE(lm.presence == Approx(0.0f));
}

TEST_CASE("Landmark can be constructed with values", "[LandmarkData]")
{
    Landmark lm{0.5f, 0.3f, -0.1f, 0.9f, 0.8f};
    REQUIRE(lm.x == Approx(0.5f));
    REQUIRE(lm.y == Approx(0.3f));
    REQUIRE(lm.z == Approx(-0.1f));
    REQUIRE(lm.visibility == Approx(0.9f));
    REQUIRE(lm.presence == Approx(0.8f));
}

TEST_CASE("BoundingBox defaults to zero", "[LandmarkData]")
{
    BoundingBox box;
    REQUIRE(box.cx == Approx(0.0f));
    REQUIRE(box.cy == Approx(0.0f));
    REQUIRE(box.width == Approx(0.0f));
    REQUIRE(box.height == Approx(0.0f));
    REQUIRE(box.confidence == Approx(0.0f));
}

TEST_CASE("HandLandmarks invalid when empty", "[LandmarkData]")
{
    HandLandmarks hand;
    REQUIRE_FALSE(hand.valid());
    REQUIRE(hand.confidence() == Approx(0.0f));
}

TEST_CASE("HandLandmarks valid with 21 landmarks and high confidence",
          "[LandmarkData]")
{
    HandLandmarks hand;
    hand.landmarks.resize(21);
    hand.palmBox.confidence = 0.9f;
    hand.palmBox.width = 0.2f;
    hand.palmBox.height = 0.2f;
    hand.isLeft = true;
    hand.handednessScore = 0.3f;

    REQUIRE(hand.valid());
    REQUIRE(hand.confidence() == Approx(0.9f));
}

TEST_CASE("HandLandmarks invalid with wrong landmark count", "[LandmarkData]")
{
    HandLandmarks hand;
    hand.landmarks.resize(10);
    hand.palmBox.confidence = 0.9f;
    REQUIRE_FALSE(hand.valid());
}

TEST_CASE("PoseLandmarks invalid when empty", "[LandmarkData]")
{
    PoseLandmarks pose;
    REQUIRE_FALSE(pose.valid());
}

TEST_CASE("PoseLandmarks valid with 33 landmarks", "[LandmarkData]")
{
    PoseLandmarks pose;
    pose.landmarks.resize(33);
    REQUIRE(pose.valid());
}

TEST_CASE("FrameData reports hands correctly", "[LandmarkData]")
{
    FrameData frame;
    REQUIRE_FALSE(frame.hasHands());
    REQUIRE_FALSE(frame.hasPose());

    HandLandmarks hand;
    hand.landmarks.resize(21);
    hand.palmBox.confidence = 0.9f;
    hand.palmBox.width = 0.2f;
    hand.palmBox.height = 0.2f;
    frame.hands.push_back(hand);

    REQUIRE(frame.hasHands());
    REQUIRE_FALSE(frame.hasPose());
}

TEST_CASE("DetectedPalm stores keypoints correctly", "[LandmarkData]")
{
    DetectedPalm palm;
    palm.box = {0.5f, 0.5f, 0.3f, 0.4f, 0.95f};
    palm.keypoints = {{0.4f, 0.5f}, {0.6f, 0.5f}};

    REQUIRE(palm.box.cx == Approx(0.5f));
    REQUIRE(palm.box.confidence == Approx(0.95f));
    REQUIRE(palm.keypoints.size() == 2);
    REQUIRE(palm.keypoints[0].first == Approx(0.4f));
}

// ── TFLiteRuntime Tests ──────────────────────────────────────────────

TEST_CASE("TFLiteRuntime reports error for missing model", "[TFLiteRuntime]")
{
    TFLiteRuntime runtime;
    auto err = runtime.load("/nonexistent/palm_detection_lite.tflite");
    REQUIRE(err == TFLiteRuntime::Error::ModelNotFound);
    REQUIRE_FALSE(runtime.isLoaded());
}

TEST_CASE("TFLiteRuntime reports error for invalid model file", "[TFLiteRuntime]")
{
    TFLiteRuntime runtime;
    auto err = runtime.load("/tmp/nonexistent_model.tflite");
    REQUIRE(err == TFLiteRuntime::Error::ModelNotFound);
}

// ── Camera Tests ──────────────────────────────────────────────────────

TEST_CASE("Camera reports error for invalid device", "[Camera]")
{
    Camera cam;
    auto err = cam.open(999, 640, 480, 30);
    REQUIRE(err == Camera::Error::NotFound);
    REQUIRE_FALSE(cam.isOpen());
}

TEST_CASE("Camera state management", "[Camera]")
{
    Camera cam;
    REQUIRE_FALSE(cam.isCapturing());

    cam.stopCapture();
    REQUIRE_FALSE(cam.isCapturing());
}

// ── PalmDetector Tests ────────────────────────────────────────────────

TEST_CASE("PalmDetector errors on missing model", "[PalmDetector]")
{
    PalmDetector detector;
    auto err = detector.load("/nonexistent/palm_detection_lite.tflite");
    REQUIRE(err == PalmDetector::Error::ModelLoadFailed);
    REQUIRE_FALSE(detector.isLoaded());
}

TEST_CASE("PalmDetector returns empty on empty input", "[PalmDetector]")
{
    PalmDetector detector;
    cv::Mat emptyFrame;
    auto result = detector.detect(emptyFrame);
    REQUIRE(result.empty());
}

// ── HandLandmarker Tests ──────────────────────────────────────────────

TEST_CASE("HandLandmarker errors on missing model", "[HandLandmarker]")
{
    HandLandmarker landmarker;
    auto err = landmarker.load("/nonexistent/hand_landmark_lite.tflite");
    REQUIRE(err == HandLandmarker::Error::ModelLoadFailed);
    REQUIRE_FALSE(landmarker.isLoaded());
}

TEST_CASE("HandLandmarker returns empty on empty input", "[HandLandmarker]")
{
    HandLandmarker landmarker;
    cv::Mat emptyFrame;
    DetectedPalm palm;
    auto result = landmarker.detect(emptyFrame, palm);
    REQUIRE_FALSE(result.valid());
}

// ── PoseLandmarker Tests ──────────────────────────────────────────────

TEST_CASE("PoseLandmarker errors on missing model", "[PoseLandmarker]")
{
    PoseLandmarker landmarker;
    auto err = landmarker.load("/nonexistent/pose_landmark_lite.tflite");
    REQUIRE(err == PoseLandmarker::Error::ModelLoadFailed);
    REQUIRE_FALSE(landmarker.isLoaded());
}

// ── Edge Case Tests ───────────────────────────────────────────────────

TEST_CASE("Landmark tolerates NaN values gracefully", "[EdgeCase]")
{
    Landmark lm;
    lm.x = std::nanf("");
    lm.y = std::nanf("");

    REQUIRE(std::isnan(lm.x));
    REQUIRE(std::isnan(lm.y));
}

TEST_CASE("HandLandmarks with extreme confidence values", "[EdgeCase]")
{
    HandLandmarks hand;
    hand.palmBox.confidence = 1.5f;
    REQUIRE_FALSE(hand.valid());

    hand.landmarks.resize(21);
    hand.palmBox.width = 0.2f;
    hand.palmBox.height = 0.2f;
    REQUIRE(hand.valid());
    REQUIRE(hand.confidence() == Approx(1.5f));
}

TEST_CASE("FrameData with multiple hands", "[EdgeCase]")
{
    FrameData frame;
    for (int i = 0; i < 5; ++i)
    {
        HandLandmarks hand;
        hand.landmarks.resize(21);
        hand.palmBox.confidence = 0.5f + static_cast<float>(i) * 0.1f;
        hand.palmBox.width = 0.2f;
        hand.palmBox.height = 0.2f;
        frame.hands.push_back(hand);
    }

    REQUIRE(frame.hasHands());
    REQUIRE(frame.hands.size() == 5);
}

TEST_CASE("Empty DetectedPalm keypoints", "[EdgeCase]")
{
    DetectedPalm palm;
    palm.box = {0.5f, 0.5f, 0.2f, 0.2f, 0.8f};
    REQUIRE(palm.keypoints.empty());
    REQUIRE(palm.box.cx == Approx(0.5f));
}

TEST_CASE("TFLiteRuntime unload safety", "[EdgeCase]")
{
    TFLiteRuntime runtime;
    runtime.unload();
    REQUIRE_FALSE(runtime.isLoaded());

    auto* ptr = runtime.getOutputFloatPtr(0);
    REQUIRE(ptr == nullptr);
}

TEST_CASE("Camera multiple stop safety", "[EdgeCase]")
{
    Camera cam;
    cam.stopCapture();
    cam.stopCapture();
    REQUIRE_FALSE(cam.isCapturing());
    REQUIRE_FALSE(cam.isOpen());
}

// ── PalmDetector Threshold Tests ───────────────────────────────────────

TEST_CASE("PalmDetector confidence threshold is 0.40",
          "[PalmDetector][Threshold]")
{
    REQUIRE(PalmDetector::kConfidenceThreshold == Approx(0.40f));
}

TEST_CASE("PalmDetector tracking threshold is lower than main threshold",
          "[PalmDetector][Threshold]")
{
    REQUIRE(PalmDetector::kTrackingConfidenceThreshold
            < PalmDetector::kConfidenceThreshold);
    REQUIRE(PalmDetector::kTrackingConfidenceThreshold == Approx(0.30f));
}

TEST_CASE("HandLandmarks valid() accepts confidence above valid threshold",
          "[HandLandmarks][Threshold]")
{
    HandLandmarks hand;
    hand.landmarks.resize(21);
    hand.palmBox.confidence = 0.5f;
    hand.palmBox.width = 0.2f;
    hand.palmBox.height = 0.2f;
    REQUIRE(hand.valid());
}

TEST_CASE("HandLandmarks valid() rejects confidence below valid threshold",
          "[HandLandmarks][Threshold]")
{
    HandLandmarks hand;
    hand.landmarks.resize(21);
    hand.palmBox.confidence = 0.30f;
    REQUIRE_FALSE(hand.valid());
}
