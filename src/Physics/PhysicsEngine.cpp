#include "PhysicsEngine.h"
#include <algorithm>
#include <chrono>

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

        FrameData frame;
        {
            std::lock_guard<std::mutex> lock(frameMutex);
            frame = latestFrame;
        }

        processFrame(frame);

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

    if (!frame.hasHands())
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
        std::lock_guard<std::mutex> lock(stateMutex);
        currentState = newState;
        return;
    }

    newState.tracking = true;

    auto assignment = assignHands(frame);

    if (assignment.fretHand)
    {
        fretboard.update(*assignment.fretHand, cal);
        newState.fretState = fretboard.getState();
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

    if (frame.hands.size() == 1)
    {
        auto& hand = frame.hands[0];
        if (calibration.leftHandFretting)
        {
            if (hand.isLeft)
                assignment.fretHand = &hand;
            else
                assignment.strumHand = &hand;
        }
        else
        {
            if (hand.isLeft)
                assignment.strumHand = &hand;
            else
                assignment.fretHand = &hand;
        }
    }
    else if (frame.hands.size() >= 2)
    {
        for (auto& hand : frame.hands)
        {
            if (hand.isLeft == calibration.leftHandFretting)
                assignment.fretHand = &hand;
            else
                assignment.strumHand = &hand;
        }
    }

    return assignment;
}

} // namespace AirGuitar
