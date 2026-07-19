#include "HandPipeline.h"
#include <algorithm>
#include <iostream>

namespace AirGuitar {

HandPipeline::~HandPipeline()
{
    stop();
}

HandPipeline::Error HandPipeline::initialise(const std::string& dir)
{
    if (initialised.load())
        return Error::AlreadyInitialised;

    modelsDir = dir;

    auto palmPath = dir + "/palm_detection_lite.tflite";
    auto handPath = dir + "/hand_landmark_lite.tflite";
    auto posePath = dir + "/pose_landmark_lite.tflite";

    palmDetector = std::make_unique<PalmDetector>();
    auto palmErr = palmDetector->load(palmPath);
    if (palmErr != PalmDetector::Error::None)
        return Error::PalmModelLoadFailed;

    handLandmarker = std::make_unique<HandLandmarker>();
    auto handErr = handLandmarker->load(handPath);
    if (handErr != HandLandmarker::Error::None)
        return Error::HandModelLoadFailed;

    poseLandmarker = std::make_unique<PoseLandmarker>();
    auto poseErr = poseLandmarker->load(posePath);
    if (poseErr != PoseLandmarker::Error::None)
        return Error::PoseModelLoadFailed;

    initialised.store(true);
    return Error::None;
}

void HandPipeline::start()
{
    if (!initialised.load() || running.load())
        return;

    shouldStop.store(false);
    running.store(true);

    inferenceThread = std::thread(&HandPipeline::inferenceLoop, this);
}

void HandPipeline::stop()
{
    if (!running.load())
        return;

    shouldStop.store(true);
    if (inferenceThread.joinable())
        inferenceThread.join();
    running.store(false);
}

bool HandPipeline::isRunning() const
{
    return running.load();
}

bool HandPipeline::isInitialised() const
{
    return initialised.load();
}

FrameData HandPipeline::getLatestLandmarks()
{
    std::lock_guard<std::mutex> lock(resultMutex);
    return lastResultWithHands;
}

double HandPipeline::getInferenceRate() const
{
    return measuredInferenceRate;
}

void HandPipeline::setFrameCallback(FrameCallback cb)
{
    frameCallback = std::move(cb);
}

void HandPipeline::onFrameReceived(const cv::Mat& frame, int64_t timestampMs)
{
    if (!running.load() || frame.empty())
        return;

    std::lock_guard<std::mutex> lock(frameQueueMutex);

    if (frameQueue.size() < 3)
    {
        frameQueue.push_back({frame.clone(), timestampMs});
    }
}

void HandPipeline::inferenceLoop()
{
    lastFpsMeasurement = std::chrono::steady_clock::now();

    while (!shouldStop.load())
    {
        QueuedFrame qf;
        {
            std::lock_guard<std::mutex> lock(frameQueueMutex);
            if (frameQueue.empty())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            qf = frameQueue.front();
            frameQueue.erase(frameQueue.begin());
        }

        processFrame(qf.frame, qf.timestampMs);

        ++inferenceCount;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastFpsMeasurement).count();

        if (elapsed >= 1000)
        {
            measuredInferenceRate = static_cast<double>(inferenceCount)
                                    * 1000.0 / static_cast<double>(elapsed);
            inferenceCount = 0;
            lastFpsMeasurement = now;
        }
    }
}

void HandPipeline::processFrame(const cv::Mat& frame, int64_t timestampMs)
{
    FrameData frameData;
    frameData.timestampMs = timestampMs;

    if (poseLandmarker)
    {
        frameData.pose = poseLandmarker->detect(frame);
    }

    if (palmDetector && handLandmarker)
    {
        ++frameSkipCounter;
        ++framesSinceLastPalmDetection;
        std::vector<DetectedPalm> palms;

        bool isDetectionFrame = (frameSkipCounter >= kPalmDetectionInterval);

        if (isDetectionFrame)
        {
            palms = palmDetector->detect(frame);
            frameSkipCounter = 0;

            static int palmLogCounter = 0;
            if (++palmLogCounter % 15 == 0)
            {
                std::cout << "[HandPipeline] palms=" << palms.size();
                for (size_t i = 0; i < palms.size(); ++i)
                {
                    auto& p = palms[i];
                    std::cout << " p" << i << "=[cx=" << p.box.cx
                              << " cy=" << p.box.cy
                              << " w=" << p.box.width
                              << " h=" << p.box.height
                              << " conf=" << p.box.confidence << "]";
                }
                std::cout << std::endl;
            }

            if (!palms.empty())
            {
                trackedPalms = palms;
                framesSinceLastPalmDetection = 0;
            }
        }

        for (const auto& palm : palms)
        {
            auto hand = handLandmarker->detect(frame, palm);
            if (hand.valid())
                frameData.hands.push_back(hand);
        }

        if (frameData.hands.empty() && !trackedPalms.empty()
            && framesSinceLastPalmDetection <= kMaxTrackingFrames)
        {
            for (auto& palm : trackedPalms)
            {
                DetectedPalm expanded = palm;
                expanded.box.width *= (1.0f + kTrackingBoxExpansion);
                expanded.box.height *= (1.0f + kTrackingBoxExpansion);
                auto hand = handLandmarker->detect(frame, expanded);
                if (hand.valid())
                {
                    frameData.hands.push_back(hand);
                    updateTrackedPalm(palm, hand);
                }
            }
        }

        if (framesSinceLastPalmDetection > kMaxTrackingFrames)
            trackedPalms.clear();

        if (frameData.hands.size() > PalmDetector::kMaxDetections)
        {
            std::sort(frameData.hands.begin(), frameData.hands.end(),
                [](const HandLandmarks& a, const HandLandmarks& b) {
                    return a.confidence() > b.confidence();
                });
            frameData.hands.resize(PalmDetector::kMaxDetections);
        }

        static int logCounter = 0;
        if (++logCounter % 30 == 0)
        {
            std::cout << "[HandPipeline] palms=" << palms.size()
                      << " hands=" << frameData.hands.size()
                      << " tracked=" << trackedPalms.size()
                      << " sinceDetect=" << framesSinceLastPalmDetection
                      << std::endl;
        }
    }

    {
        std::lock_guard<std::mutex> lock(resultMutex);
        latestResult = frameData;
        if (frameData.hasHands())
            lastResultWithHands = frameData;
    }

    if (frameCallback)
        frameCallback(frameData);
}

std::vector<DetectedPalm> HandPipeline::expandPalmBox(const DetectedPalm& palm, float expansion)
{
    DetectedPalm expanded = palm;
    expanded.box.width *= (1.0f + expansion);
    expanded.box.height *= (1.0f + expansion);
    return {expanded};
}

bool HandPipeline::updateTrackedPalm(DetectedPalm& palm, const HandLandmarks& hand)
{
    if (hand.landmarks.empty())
        return false;

    float sumX = 0.0f, sumY = 0.0f;
    for (const auto& lm : hand.landmarks)
    {
        sumX += lm.x;
        sumY += lm.y;
    }
    float n = static_cast<float>(hand.landmarks.size());
    palm.box.cx = sumX / n;
    palm.box.cy = sumY / n;
    return true;
}

} // namespace AirGuitar
