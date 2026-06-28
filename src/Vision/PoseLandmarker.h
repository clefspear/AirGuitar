#pragma once
#include "TFLiteRuntime.h"
#include "LandmarkData.h"
#include <opencv2/core.hpp>
#include <vector>
#include <memory>

namespace AirGuitar {

class PoseLandmarker
{
public:
    enum class Error
    {
        None,
        ModelLoadFailed,
        InferenceFailed,
        InvalidInput,
    };

    static constexpr int kInputSize = 256;
    static constexpr int kNumLandmarks = 33;

    PoseLandmarker() = default;
    ~PoseLandmarker() = default;

    PoseLandmarker(const PoseLandmarker&) = delete;
    PoseLandmarker& operator=(const PoseLandmarker&) = delete;
    PoseLandmarker(PoseLandmarker&&) = default;
    PoseLandmarker& operator=(PoseLandmarker&&) = default;

    Error load(const std::string& modelPath);
    bool isLoaded() const;

    PoseLandmarks detect(const cv::Mat& frame);

private:
    std::unique_ptr<TFLiteRuntime> runtime;

    cv::Mat preprocess(const cv::Mat& frame, float& padX, float& padY,
                       float& scale);
    PoseLandmarks decodeOutput(const float* output, float padX, float padY,
                                float scale, const cv::Size& originalSize);
};

} // namespace AirGuitar
