#pragma once
#include "PhysicsData.h"
#include "CalibrationData.h"
#include "FretboardTracker.h"
#include "StrumDetector.h"
#include "ChordClassifier.h"
#include "Vision/LandmarkData.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <vector>

namespace AirGuitar {

class PhysicsEngine
{
public:
    using NoteCallback = std::function<void(const NoteEvent&)>;

    PhysicsEngine();
    ~PhysicsEngine();

    PhysicsEngine(const PhysicsEngine&) = delete;
    PhysicsEngine& operator=(const PhysicsEngine&) = delete;

    void start();
    void stop();
    bool isRunning() const { return running.load(); }

    void pushFrame(const FrameData& frame);
    void setCalibration(const CalibrationData& cal);
    void setNoteCallback(NoteCallback cb);

    PhysicsState getLatestState() const;

private:
    void physicsLoop();
    void processFrame(const FrameData& frame);
    HandAssignment assignHands(const FrameData& frame);

    CalibrationData calibration;
    mutable std::mutex stateMutex;
    FrameData latestFrame;
    mutable std::mutex frameMutex;

    FretboardTracker fretboard;
    StrumDetector strumDetector;
    ChordClassifier chordClassifier;
    PhysicsState currentState;

    NoteCallback noteCallback;

    std::thread physicsThread;
    std::atomic<bool> running{false};
    std::atomic<bool> shouldStop{false};
};

} // namespace AirGuitar
