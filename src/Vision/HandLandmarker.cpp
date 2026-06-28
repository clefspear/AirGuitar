#include "HandLandmarker.h"
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <numbers>
#include <chrono>
#include <cstring>

namespace AirGuitar {

HandLandmarker::Error HandLandmarker::load(const std::string& modelPath)
{
    runtime = std::make_unique<TFLiteRuntime>();
    auto err = runtime->load(modelPath, 2);
    if (err != TFLiteRuntime::Error::None)
        return Error::ModelLoadFailed;

    auto dims = runtime->getInputDims(0);
    if (dims.empty())
        return Error::ModelLoadFailed;

    return Error::None;
}

bool HandLandmarker::isLoaded() const
{
    return runtime && runtime->isLoaded();
}

HandLandmarks HandLandmarker::detect(const cv::Mat& frame,
                                      const DetectedPalm& palm)
{
    HandLandmarks result;
    if (!isLoaded() || frame.empty())
        return result;

    cv::Mat rotationMatrix;
    auto croppedWarped = preprocess(frame, palm, rotationMatrix);

    auto* inputData = runtime->getInputFloatPtr(0);
    if (!inputData)
        return result;

    auto* inputBlob = croppedWarped.ptr<float>();
    std::memcpy(inputData, inputBlob,
                kInputSize * kInputSize * 3 * sizeof(float));

    auto inferErr = runtime->runInference();
    if (inferErr != TFLiteRuntime::Error::None)
        return result;

    auto* outputData = runtime->getOutputFloatPtr(0);
    if (!outputData)
        return result;

    float* handednessData = nullptr;
    if (runtime->getOutputCount() > 1)
        handednessData = runtime->getOutputFloatPtr(1);

    auto originalSize = frame.size();
    cv::Rect cropRect(
        static_cast<int>(palm.box.cx - palm.box.width / 2.0f),
        static_cast<int>(palm.box.cy - palm.box.height / 2.0f),
        static_cast<int>(palm.box.width),
        static_cast<int>(palm.box.height));

    result = decodeOutput(outputData, handednessData,
                          rotationMatrix, cropRect, originalSize);
    result.palmBox = palm.box;
    result.timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    return result;
}

cv::Mat HandLandmarker::preprocess(const cv::Mat& frame,
                                    const DetectedPalm& palm,
                                    cv::Mat& rotationMatrix)
{
    float cx = palm.box.cx;
    float cy = palm.box.cy;
    float w = palm.box.width;
    float h = palm.box.height;

    cv::Point2f center(cx, cy);
    float size = std::max(w, h) * 1.5f;

    float rotation = 0.0f;
    if (palm.keypoints.size() >= 2)
    {
        float dx = palm.keypoints[1].first - palm.keypoints[0].first;
        float dy = palm.keypoints[1].second - palm.keypoints[0].second;
        rotation = std::atan2(dy, dx) * 180.0f
                   / static_cast<float>(std::numbers::pi);
    }

    cv::Mat rot = cv::getRotationMatrix2D(center, rotation, 1.0f);
    rotationMatrix = rot.clone();

    cv::Mat rotated;
    cv::warpAffine(frame, rotated, rot, frame.size(), cv::INTER_LINEAR,
                   cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    int x = std::max(0, static_cast<int>(center.x - size / 2.0f));
    int y = std::max(0, static_cast<int>(center.y - size / 2.0f));
    int cropW = std::min(static_cast<int>(size),
                         rotated.cols - x);
    int cropH = std::min(static_cast<int>(size),
                         rotated.rows - y);

    cv::Rect cropRect(x, y, cropW, cropH);
    cv::Mat cropped = rotated(cropRect).clone();

    cv::Mat resized;
    cv::resize(cropped, resized, cv::Size(kInputSize, kInputSize), 0, 0,
               cv::INTER_LINEAR);

    cv::Mat normalized;
    resized.convertTo(normalized, CV_32FC3, 2.0f / 255.0f, -1.0f);

    return normalized;
}

HandLandmarks HandLandmarker::decodeOutput(
    const float* output, const float* handedness,
    const cv::Mat& rotationMatrix,
    const cv::Rect& cropRect,
    const cv::Size& originalSize)
{
    HandLandmarks result;
    result.landmarks.reserve(kNumLandmarks);

    cv::Mat invRot;
    cv::invertAffineTransform(rotationMatrix, invRot);

    float cropCenterX = cropRect.x + cropRect.width / 2.0f;
    float cropCenterY = cropRect.y + cropRect.height / 2.0f;
    float cropSize = std::max(cropRect.width, cropRect.height);

    for (int i = 0; i < kNumLandmarks; ++i)
    {
        Landmark lm;
        lm.x = output[i * 3 + 0];
        lm.y = output[i * 3 + 1];
        lm.z = output[i * 3 + 2];

        float imgX = lm.x * kInputSize / static_cast<float>(kInputSize)
                     * cropSize + cropCenterX;
        float imgY = lm.y * kInputSize / static_cast<float>(kInputSize)
                     * cropSize + cropCenterY;

        cv::Mat pt(3, 1, CV_32F);
        pt.at<float>(0) = imgX;
        pt.at<float>(1) = imgY;
        pt.at<float>(2) = 1.0f;

        cv::Mat origPt = invRot * pt;

        lm.x = origPt.at<float>(0) / static_cast<float>(originalSize.width);
        lm.y = origPt.at<float>(1) / static_cast<float>(originalSize.height);
        lm.z = lm.z / static_cast<float>(kInputSize) * cropSize
               / static_cast<float>(originalSize.width);
        lm.visibility = 1.0f;
        lm.presence = 1.0f;

        result.landmarks.push_back(lm);
    }

    if (handedness != nullptr)
    {
        result.handednessScore = handedness[0];
        result.isLeft = handedness[0] < 0.5f;
    }

    return result;
}

} // namespace AirGuitar
