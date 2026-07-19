#pragma once
#include "LandmarkData.h"
#include "Camera.h"
#include "PalmDetector.h"
#include "HandLandmarker.h"
#include "PoseLandmarker.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <memory>
#include <chrono>
#include <functional>

namespace AirGuitar {

class HandPipeline
{
public:
    using FrameCallback = std::function<void(const FrameData&)>;

    enum class Error
    {
        None,
        PalmModelLoadFailed,
        HandModelLoadFailed,
        PoseModelLoadFailed,
        AlreadyInitialised,
        AlreadyRunning,
        NotRunning,
    };

    HandPipeline() = default;
    ~HandPipeline();

    HandPipeline(const HandPipeline&) = delete;
    HandPipeline& operator=(const HandPipeline&) = delete;
    HandPipeline(HandPipeline&&) = delete;
    HandPipeline& operator=(HandPipeline&&) = delete;

    Error initialise(const std::string& modelsDir);
    void start();
    void stop();
    bool isRunning() const;
    bool isInitialised() const;

    FrameData getLatestLandmarks();
    double getInferenceRate() const;

    void setFrameCallback(FrameCallback cb);
    void onFrameReceived(const cv::Mat& frame, int64_t timestampMs);

private:
    static constexpr int kPalmDetectionInterval = 3;
    static constexpr int kMaxTrackingFrames = 15;
    static constexpr float kTrackingBoxExpansion = 0.3f;

    std::unique_ptr<PalmDetector> palmDetector;
    std::unique_ptr<HandLandmarker> handLandmarker;
    std::unique_ptr<PoseLandmarker> poseLandmarker;

    std::thread inferenceThread;
    std::mutex resultMutex;
    std::mutex frameQueueMutex;

    std::atomic<bool> running{false};
    std::atomic<bool> shouldStop{false};
    std::atomic<bool> initialised{false};

    FrameData latestResult;
    FrameData lastResultWithHands;

    int inferenceCount = 0;
    int frameSkipCounter = 0;
    double measuredInferenceRate = 0.0;
    std::chrono::steady_clock::time_point lastFpsMeasurement;

    struct QueuedFrame
    {
        cv::Mat frame;
        int64_t timestampMs;
    };
    std::vector<QueuedFrame> frameQueue;
    std::vector<DetectedPalm> trackedPalms;
    int framesSinceLastPalmDetection = 0;
    FrameCallback frameCallback;

    void inferenceLoop();
    void processFrame(const cv::Mat& frame, int64_t timestampMs);
    std::vector<DetectedPalm> expandPalmBox(const DetectedPalm& palm, float expansion);
    bool updateTrackedPalm(DetectedPalm& palm, const HandLandmarks& hand);

    std::string modelsDir;
};

} // namespace AirGuitar
