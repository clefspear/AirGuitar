#include "PalmDetector.h"
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <numeric>

namespace AirGuitar {

static constexpr float kAnchorStrides[] = {8.0f, 16.0f};
static constexpr float kAnchorScales[] = {
    0.1484375f, 0.210f, 0.297f, 0.420f
};
static constexpr float kAnchorOffset = 0.5f;
static constexpr int kNumLayers = 2;
static constexpr int kNumScalesPerLayer = 2;
static constexpr float kRegressionScale = 0.1f;

PalmDetector::Error PalmDetector::load(const std::string& modelPath)
{
    runtime = std::make_unique<TFLiteRuntime>();
    auto err = runtime->load(modelPath, 2);
    if (err != TFLiteRuntime::Error::None)
        return Error::ModelLoadFailed;

    generateAnchors();
    return Error::None;
}

bool PalmDetector::isLoaded() const
{
    return runtime && runtime->isLoaded();
}

std::vector<DetectedPalm> PalmDetector::detect(const cv::Mat& frame)
{
    return detect(frame, kConfidenceThreshold);
}

std::vector<DetectedPalm> PalmDetector::detect(const cv::Mat& frame, float confidenceThreshold)
{
    if (!isLoaded() || frame.empty())
        return {};

    float padX = 0.0f, padY = 0.0f, scale = 1.0f;
    auto inputTensor = preprocess(frame, padX, padY, scale);

    auto* inputData = runtime->getInputFloatPtr(0);
    if (!inputData)
        return {};

    std::memcpy(inputData, inputTensor.ptr<float>(),
                kInputSize * kInputSize * 3 * sizeof(float));

    auto inferErr = runtime->runInference();
    if (inferErr != TFLiteRuntime::Error::None)
        return {};

    auto* outputData = runtime->getOutputFloatPtr(0);
    if (!outputData)
        return {};

    auto detections = decodeOutput(outputData, padX, padY, scale, confidenceThreshold);
    return nonMaxSuppression(std::move(detections), kNmsThreshold);
}

cv::Mat PalmDetector::preprocess(const cv::Mat& frame,
                                  float& padX, float& padY,
                                  float& scale)
{
    int h = frame.rows;
    int w = frame.cols;
    int size = std::max(w, h);

    float scaleX = static_cast<float>(kInputSize) / static_cast<float>(size);
    float scaleY = scaleX;
    scale = scaleX;

    int paddedW = static_cast<int>(size * scaleX);
    int paddedH = static_cast<int>(size * scaleY);

    padX = (paddedW - w * scaleX) / 2.0f;
    padY = (paddedH - h * scaleY) / 2.0f;

    cv::Mat padded;
    int top = static_cast<int>(padY);
    int bottom = static_cast<int>(padY);
    int left = static_cast<int>(padX);
    int right = static_cast<int>(padX);
    cv::copyMakeBorder(frame, padded, top, bottom, left, right,
                        cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    cv::Mat resized;
    cv::resize(padded, resized, cv::Size(kInputSize, kInputSize), 0, 0,
               cv::INTER_LINEAR);

    cv::Mat normalized;
    resized.convertTo(normalized, CV_32FC3, 2.0f / 255.0f, -1.0f);

    return normalized;
}

void PalmDetector::generateAnchors()
{
    anchors.clear();
    anchors.reserve(kNumAnchors);

    for (int layer = 0; layer < kNumLayers; ++layer)
    {
        float stride = kAnchorStrides[layer];
        int gridSize = static_cast<int>(std::ceil(
            static_cast<float>(kInputSize) / stride));

        for (int y = 0; y < gridSize; ++y)
        {
            for (int x = 0; x < gridSize; ++x)
            {
                for (int s = 0; s < kNumScalesPerLayer; ++s)
                {
                    int scaleIdx = layer * kNumScalesPerLayer + s;
                    float scale = kAnchorScales[scaleIdx];

                    float cx = (static_cast<float>(x) + kAnchorOffset)
                               / static_cast<float>(gridSize);
                    float cy = (static_cast<float>(y) + kAnchorOffset)
                               / static_cast<float>(gridSize);
                    float w = scale;
                    float h = scale;

                    DetectedPalm anchor;
                    anchor.box.cx = cx;
                    anchor.box.cy = cy;
                    anchor.box.width = w;
                    anchor.box.height = h;
                    anchors.push_back(anchor);
                }
            }
        }
    }
}

std::vector<DetectedPalm> PalmDetector::decodeOutput(
    const float* output, float padX, float padY, float scale,
    float confidenceThreshold)
{
    std::vector<DetectedPalm> results;
    results.reserve(kNumAnchors);

    int numAnchors = std::min(kNumAnchors,
        static_cast<int>(runtime->getOutputDims(0)[1]));

    for (int i = 0; i < numAnchors; ++i)
    {
        const float* raw = output + i * 19;

        float confidence = sigmoid(raw[4]);
        if (confidence < confidenceThreshold)
            continue;

        float cx = raw[0] * kRegressionScale * anchors[i].box.width
                   + anchors[i].box.cx;
        float cy = raw[1] * kRegressionScale * anchors[i].box.height
                   + anchors[i].box.cy;
        float w = std::exp(raw[2] * kRegressionScale) * anchors[i].box.width;
        float h = std::exp(raw[3] * kRegressionScale) * anchors[i].box.height;

        DetectedPalm palm;
        palm.box.cx = (cx - padX / kInputSize) / scale;
        palm.box.cy = (cy - padY / kInputSize) / scale;
        palm.box.width = w / scale;
        palm.box.height = h / scale;
        palm.box.confidence = confidence;
        palm.handId = 0;

        for (int kp = 0; kp < 7; ++kp)
        {
            float kpX = raw[5 + kp * 2] * kRegressionScale
                        * anchors[i].box.width + anchors[i].box.cx;
            float kpY = raw[6 + kp * 2] * kRegressionScale
                        * anchors[i].box.height + anchors[i].box.cy;
            kpX = (kpX - padX / kInputSize) / scale;
            kpY = (kpY - padY / kInputSize) / scale;
            palm.keypoints.emplace_back(kpX, kpY);
        }

        results.push_back(palm);
    }

    std::sort(results.begin(), results.end(),
              [](const DetectedPalm& a, const DetectedPalm& b) {
                  return a.box.confidence > b.box.confidence;
              });

    if (results.size() > kMaxDetections)
        results.resize(kMaxDetections);

    return results;
}

std::vector<DetectedPalm> PalmDetector::nonMaxSuppression(
    std::vector<DetectedPalm> detections, float iouThreshold)
{
    if (detections.empty())
        return {};

    std::vector<bool> suppressed(detections.size(), false);
    std::vector<DetectedPalm> kept;

    for (size_t i = 0; i < detections.size(); ++i)
    {
        if (suppressed[i])
            continue;
        kept.push_back(detections[i]);

        for (size_t j = i + 1; j < detections.size(); ++j)
        {
            if (suppressed[j])
                continue;

            float iou = computeIoU(detections[i].box, detections[j].box);
            if (iou > iouThreshold)
                suppressed[j] = true;
        }
    }

    return kept;
}

float PalmDetector::computeIoU(const BoundingBox& a, const BoundingBox& b)
{
    float ax1 = a.cx - a.width / 2.0f;
    float ay1 = a.cy - a.height / 2.0f;
    float ax2 = a.cx + a.width / 2.0f;
    float ay2 = a.cy + a.height / 2.0f;

    float bx1 = b.cx - b.width / 2.0f;
    float by1 = b.cy - b.height / 2.0f;
    float bx2 = b.cx + b.width / 2.0f;
    float by2 = b.cy + b.height / 2.0f;

    float x1 = std::max(ax1, bx1);
    float y1 = std::max(ay1, by1);
    float x2 = std::min(ax2, bx2);
    float y2 = std::min(ay2, by2);

    float interArea = std::max(0.0f, x2 - x1) * std::max(0.0f, y2 - y1);
    float boxAArea = (ax2 - ax1) * (ay2 - ay1);
    float boxBArea = (bx2 - bx1) * (by2 - by1);

    float unionArea = boxAArea + boxBArea - interArea;
    if (unionArea <= 0.0f) return 0.0f;

    return interArea / unionArea;
}

float PalmDetector::sigmoid(float x)
{
    return 1.0f / (1.0f + std::exp(-x));
}

} // namespace AirGuitar
