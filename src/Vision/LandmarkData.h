#pragma once
#include <vector>
#include <cstdint>
#include <string>

struct Landmark
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float visibility = 0.0f;
    float presence = 0.0f;
};

struct BoundingBox
{
    float cx = 0.0f;
    float cy = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float confidence = 0.0f;
};

struct HandLandmarks
{
    std::vector<Landmark> landmarks;
    BoundingBox palmBox;
    bool isLeft = false;
    float handednessScore = 0.0f;
    int64_t timestampMs = 0;

    bool valid() const { return landmarks.size() == 21 && palmBox.confidence > 0.5f; }
    float confidence() const { return palmBox.confidence; }
};

struct PoseLandmarks
{
    std::vector<Landmark> landmarks;
    int64_t timestampMs = 0;

    bool valid() const { return landmarks.size() == 33; }
};

struct FrameData
{
    std::vector<HandLandmarks> hands;
    PoseLandmarks pose;
    int64_t timestampMs = 0;

    bool hasHands() const { return !hands.empty(); }
    bool hasPose() const { return pose.valid(); }
};

struct DetectedPalm
{
    BoundingBox box;
    std::vector<std::pair<float, float>> keypoints;
    int handId = 0;
};
