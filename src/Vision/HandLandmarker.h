#pragma once
#include "TFLiteRuntime.h"
#include "LandmarkData.h"
#include <opencv2/core.hpp>
#include <vector>
#include <memory>

namespace AirGuitar {

class HandLandmarker
{
public:
    enum class Error
    {
        None,
        ModelLoadFailed,
        InferenceFailed,
        InvalidInput,
    };

    static constexpr int kInputSize = 224;
    static constexpr int kNumLandmarks = 21;

    HandLandmarker() = default;
    ~HandLandmarker() = default;

    HandLandmarker(const HandLandmarker&) = delete;
    HandLandmarker& operator=(const HandLandmarker&) = delete;
    HandLandmarker(HandLandmarker&&) = default;
    HandLandmarker& operator=(HandLandmarker&&) = default;

    Error load(const std::string& modelPath);
    bool isLoaded() const;

    HandLandmarks detect(const cv::Mat& frame, const DetectedPalm& palm);

private:
    std::unique_ptr<TFLiteRuntime> runtime;

    cv::Mat preprocess(const cv::Mat& frame, const DetectedPalm& palm,
                       cv::Mat& rotationMatrix);
    HandLandmarks decodeOutput(const float* output, const float* handedness,
                               const cv::Mat& rotationMatrix,
                               const cv::Rect& cropRect,
                               const cv::Size& originalSize);
};

} // namespace AirGuitar
