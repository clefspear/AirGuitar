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

namespace AirGuitar {

class HandPipeline
{
public:
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

    void onFrameReceived(const cv::Mat& frame, int64_t timestampMs);

private:
    static constexpr int kPalmDetectionInterval = 3;

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

    void inferenceLoop();
    void processFrame(const cv::Mat& frame, int64_t timestampMs);

    std::string modelsDir;
};

} // namespace AirGuitar
