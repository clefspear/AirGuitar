#include "PoseLandmarker.h"
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <chrono>
#include <cstring>

namespace AirGuitar {

PoseLandmarker::Error PoseLandmarker::load(const std::string& modelPath)
{
    runtime = std::make_unique<TFLiteRuntime>();
    auto err = runtime->load(modelPath, 2);
    if (err != TFLiteRuntime::Error::None)
        return Error::ModelLoadFailed;

    return Error::None;
}

bool PoseLandmarker::isLoaded() const
{
    return runtime && runtime->isLoaded();
}

PoseLandmarks PoseLandmarker::detect(const cv::Mat& frame)
{
    PoseLandmarks result;
    if (!isLoaded() || frame.empty())
        return result;

    float padX = 0.0f, padY = 0.0f, scale = 1.0f;
    auto inputTensor = preprocess(frame, padX, padY, scale);

    auto* inputData = runtime->getInputFloatPtr(0);
    if (!inputData)
        return result;

    std::memcpy(inputData, inputTensor.ptr<float>(),
                kInputSize * kInputSize * 3 * sizeof(float));

    auto inferErr = runtime->runInference();
    if (inferErr != TFLiteRuntime::Error::None)
        return result;

    auto* outputData = runtime->getOutputFloatPtr(0);
    if (!outputData)
        return result;

    result = decodeOutput(outputData, padX, padY, scale, frame.size());
    return result;
}

cv::Mat PoseLandmarker::preprocess(const cv::Mat& frame,
                                    float& padX, float& padY,
                                    float& scale)
{
    int h = frame.rows;
    int w = frame.cols;
    int size = std::max(w, h);

    float s = static_cast<float>(kInputSize) / static_cast<float>(size);
    scale = s;

    int newW = static_cast<int>(w * s);
    int newH = static_cast<int>(h * s);

    padX = (kInputSize - newW) / 2.0f;
    padY = (kInputSize - newH) / 2.0f;

    cv::Mat resized;
    cv::resize(frame, resized, cv::Size(newW, newH), 0, 0, cv::INTER_LINEAR);

    cv::Mat padded;
    int top = static_cast<int>(padY);
    int bottom = kInputSize - newH - top;
    int left = static_cast<int>(padX);
    int right = kInputSize - newW - left;
    cv::copyMakeBorder(resized, padded, top, bottom, left, right,
                        cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    cv::Mat normalized;
    padded.convertTo(normalized, CV_32FC3, 2.0f / 255.0f, -1.0f);

    return normalized;
}

PoseLandmarks PoseLandmarker::decodeOutput(
    const float* output, float padX, float padY,
    float scale, const cv::Size& originalSize)
{
    PoseLandmarks result;
    result.landmarks.reserve(kNumLandmarks);

    auto outputDims = runtime->getOutputDims(0);
    int numValues = 1;
    for (auto d : outputDims) numValues *= d;

    int numLandmarks = numValues / 5;
    if (numLandmarks > kNumLandmarks)
        numLandmarks = kNumLandmarks;

    for (int i = 0; i < numLandmarks; ++i)
    {
        Landmark lm;
        lm.x = (output[i * 5 + 0] * kInputSize - padX) / scale
               / static_cast<float>(originalSize.width);
        lm.y = (output[i * 5 + 1] * kInputSize - padY) / scale
               / static_cast<float>(originalSize.height);
        lm.z = output[i * 5 + 2] / scale
               / static_cast<float>(originalSize.width);
        lm.visibility = output[i * 5 + 3];
        lm.presence = output[i * 5 + 4];
        result.landmarks.push_back(lm);
    }

    result.timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    return result;
}

} // namespace AirGuitar
