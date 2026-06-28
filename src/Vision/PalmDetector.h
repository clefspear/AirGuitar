#pragma once
#include "TFLiteRuntime.h"
#include "LandmarkData.h"
#include <opencv2/core.hpp>
#include <vector>
#include <memory>

namespace AirGuitar {

class PalmDetector
{
public:
    enum class Error
    {
        None,
        ModelLoadFailed,
        InferenceFailed,
        InvalidInput,
    };

    static constexpr int kInputSize = 128;
    static constexpr float kConfidenceThreshold = 0.5f;
    static constexpr float kNmsThreshold = 0.3f;
    static constexpr int kMaxDetections = 5;
    static constexpr int kNumAnchors = 896;

    PalmDetector() = default;
    ~PalmDetector() = default;

    PalmDetector(const PalmDetector&) = delete;
    PalmDetector& operator=(const PalmDetector&) = delete;
    PalmDetector(PalmDetector&&) = default;
    PalmDetector& operator=(PalmDetector&&) = default;

    Error load(const std::string& modelPath);
    bool isLoaded() const;

    std::vector<DetectedPalm> detect(const cv::Mat& frame);

private:
    std::unique_ptr<TFLiteRuntime> runtime;
    std::vector<DetectedPalm> anchors;

    cv::Mat preprocess(const cv::Mat& frame, float& padX, float& padY,
                       float& scale);
    std::vector<DetectedPalm> decodeOutput(const float* output,
                                            float padX, float padY,
                                            float scale);
    std::vector<DetectedPalm> nonMaxSuppression(
        std::vector<DetectedPalm> detections, float iouThreshold);
    float computeIoU(const BoundingBox& a, const BoundingBox& b);

    void generateAnchors();

    static float sigmoid(float x);
};

} // namespace AirGuitar
