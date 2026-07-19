#include "PhysicsEngine.h"
#include "App/CrashLogger.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>

namespace AirGuitar {

PhysicsEngine::PhysicsEngine() {}

PhysicsEngine::~PhysicsEngine()
{
    stop();
}

void PhysicsEngine::start()
{
    if (running.load())
        return;

    shouldStop.store(false);
    running.store(true);
    physicsThread = std::thread(&PhysicsEngine::physicsLoop, this);
}

void PhysicsEngine::stop()
{
    if (!running.load())
        return;

    shouldStop.store(true);
    if (physicsThread.joinable())
        physicsThread.join();
    running.store(false);
}

void PhysicsEngine::pushFrame(const FrameData& frame)
{
    std::lock_guard<std::mutex> lock(frameMutex);
    latestFrame = frame;
    newFrameAvailable.store(true, std::memory_order_release);
}

void PhysicsEngine::setCalibration(const CalibrationData& cal)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    calibration = cal;
}

void PhysicsEngine::setNoteCallback(NoteCallback cb)
{
    noteCallback = std::move(cb);
}

PhysicsState PhysicsEngine::getLatestState() const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return currentState;
}

void PhysicsEngine::physicsLoop()
{
    auto tickInterval = std::chrono::microseconds(4167);

    while (!shouldStop.load())
    {
        auto start = std::chrono::steady_clock::now();

        if (newFrameAvailable.load(std::memory_order_acquire))
        {
            FrameData frame;
            {
                std::lock_guard<std::mutex> lock(frameMutex);
                frame = latestFrame;
            }
            newFrameAvailable.store(false, std::memory_order_release);
            processFrame(frame);
        }

        auto elapsed = std::chrono::steady_clock::now() - start;
        auto remaining = tickInterval - elapsed;
        if (remaining > std::chrono::microseconds(0))
            std::this_thread::sleep_for(remaining);
    }
}

void PhysicsEngine::processFrame(const FrameData& frame)
{
    PhysicsState newState;
    newState.timestampMs = frame.timestampMs;

    CalibrationData cal;
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        cal = calibration;
    }

    static int logCounter = 0;
    bool shouldLog = (++logCounter % 60 == 1);

    if (shouldLog)
    {
        std::ostringstream ss;
        ss << "[Physics] hands=" << frame.hands.size()
           << " ts=" << frame.timestampMs;
        for (size_t i = 0; i < frame.hands.size(); ++i)
        {
            auto& h = frame.hands[i];
            ss << " hand" << i << "=" << (h.isLeft ? "L" : "R")
               << " x=" << (h.landmarks.empty() ? 0.0f : h.landmarks[0].x)
               << " y=" << (h.landmarks.empty() ? 0.0f : h.landmarks[0].y)
               << " conf=" << h.confidence();
        }
        CrashLogger::instance().logInfo(ss.str());
    }

    if (!frame.hasHands())
    {
        bool handRecentlyLost = (lastHandTimestampMs > 0
            && frame.timestampMs - lastHandTimestampMs < kHandLostTimeoutMs);

        if (!handRecentlyLost)
        {
            newState.tracking = false;

            std::vector<NoteEvent> offEvents;
            {
                std::lock_guard<std::mutex> lock(stateMutex);
                auto calCopy = calibration;
                for (int s = 0; s < 6; ++s)
                {
                    int fret = currentState.fretState.activeStrings[s];
                    if (fret >= 0)
                    {
                        NoteEvent noteOff;
                        noteOff.type = NoteEventType::NoteOff;
                        noteOff.midiNote = calCopy.midiNoteForString(s, fret);
                        noteOff.stringIndex = s;
                        noteOff.fret = fret;
                        noteOff.velocity = 0.0f;
                        noteOff.timestampMs = frame.timestampMs;
                        offEvents.push_back(noteOff);
                    }
                }
            }

            for (auto& evt : offEvents)
            {
                if (noteCallback)
                    noteCallback(evt);
            }

            strumDetector.reset();
        }

        std::lock_guard<std::mutex> lock(stateMutex);
        newState.chordName = "";
        if (handRecentlyLost)
        {
            newState.tracking = currentState.tracking;
            newState.fretState = currentState.fretState;
        }
        currentState = newState;
        return;
    }

    newState.tracking = true;
    lastHandTimestampMs = frame.timestampMs;

    auto assignment = assignHands(frame);

    if (shouldLog)
    {
        std::ostringstream as;
        as << "[Physics] assign fret=" << (assignment.fretHand ? "yes" : "no")
           << " strum=" << (assignment.strumHand ? "yes" : "no");
        CrashLogger::instance().logInfo(as.str());
    }

    if (assignment.fretHand)
    {
        float wristX = assignment.fretHand->landmarks.empty()
            ? 0.5f : assignment.fretHand->landmarks[0].x;

        bool continuityOk = (lastFretWristX < 0.0f)
            || std::abs(wristX - lastFretWristX) < 0.3f;

        if (continuityOk)
        {
            fretboard.update(*assignment.fretHand, cal);
            newState.fretState = fretboard.getState();
            lastFretWristX = wristX;
        }
        else if (currentState.tracking)
        {
            newState.fretState = currentState.fretState;
        }
    }
    else if (currentState.tracking)
    {
        newState.fretState = currentState.fretState;
        lastFretWristX = -1.0f;
    }

    if (assignment.strumHand)
    {
        auto strumEvents = strumDetector.update(
            *assignment.strumHand, cal,
            newState.fretState.fret,
            newState.fretState.activeStrings,
            frame.timestampMs);

        for (auto& evt : strumEvents)
        {
            newState.events.push_back(evt);
            newState.strumVelocity = evt.velocity;
            newState.strumDirection = evt.direction;
            newState.strumming = true;

            if (noteCallback)
                noteCallback(evt);

            std::ostringstream ss;
            ss << "[Physics] NoteEvent type=" << (evt.type == NoteEventType::NoteOn ? "ON" : "OFF")
               << " string=" << evt.stringIndex
               << " midi=" << evt.midiNote
               << " fret=" << evt.fret
               << " vel=" << evt.velocity
               << " dir=" << (evt.direction == StrumDirection::Down ? "DN" : "UP");
            CrashLogger::instance().logInfo(ss.str());
        }
    }

    newState.chordName = chordClassifier.classify(
        newState.fretState.fret,
        newState.fretState.activeStrings,
        cal).name;

    std::lock_guard<std::mutex> lock(stateMutex);
    currentState = newState;
}

HandAssignment PhysicsEngine::assignHands(const FrameData& frame)
{
    HandAssignment assignment;

    CalibrationData cal;
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        cal = calibration;
    }

    auto isInStrumZone = [&](const HandLandmarks& hand) -> bool {
        if (hand.landmarks.empty()) return false;
        float wristX = hand.landmarks[0].x;
        float wristY = hand.landmarks[0].y;
        return wristX >= cal.strumZoneLeft && wristX <= cal.strumZoneRight
            && wristY >= cal.strumZoneTop && wristY <= cal.strumZoneBottom;
    };

    auto isOnFretSide = [&](const HandLandmarks& hand) -> bool {
        if (hand.landmarks.empty()) return false;
        return hand.landmarks[0].x < cal.strumZoneLeft;
    };

    if (frame.hands.size() == 1)
    {
        auto& hand = frame.hands[0];
        if (hand.valid() && hand.landmarks.size() >= 21)
        {
            if (isInStrumZone(hand))
                assignment.strumHand = &hand;
            else if (isOnFretSide(hand))
                assignment.fretHand = &hand;
        }
    }
    else if (frame.hands.size() >= 2)
    {
        const HandLandmarks* bestFret = nullptr;
        const HandLandmarks* bestStrum = nullptr;

        for (auto& hand : frame.hands)
        {
            if (!hand.valid() || hand.landmarks.size() < 21)
                continue;

            if (isOnFretSide(hand))
            {
                if (!bestFret || hand.confidence() > bestFret->confidence())
                    bestFret = &hand;
            }
            else if (isInStrumZone(hand))
            {
                if (!bestStrum || hand.confidence() > bestStrum->confidence())
                    bestStrum = &hand;
            }
        }

        assignment.fretHand = bestFret;
        assignment.strumHand = bestStrum;
    }

    return assignment;
}

} // namespace AirGuitar
