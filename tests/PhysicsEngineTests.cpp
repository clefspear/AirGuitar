#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Physics/PhysicsEngine.h"
#include "Physics/CalibrationData.h"

using namespace AirGuitar;
using Catch::Approx;

static HandLandmarks makeHand(float cx, float cy, bool isLeft, const CalibrationData& cal)
{
    HandLandmarks h;
    h.landmarks.resize(21);
    h.landmarks[0] = {cx, cy, 0.0f, 0.9f, 0.9f};
    h.isLeft = isLeft;
    h.palmBox.confidence = 0.9f;
    h.timestampMs = 1000;
    return h;
}

TEST_CASE("PhysicsEngine starts and stops cleanly", "[PhysicsEngine]")
{
    PhysicsEngine engine;
    engine.start();
    REQUIRE(engine.isRunning());
    engine.stop();
    REQUIRE_FALSE(engine.isRunning());
}

TEST_CASE("PhysicsEngine processes frames without crashing", "[PhysicsEngine]")
{
    PhysicsEngine engine;
    CalibrationData cal = CalibrationData::defaultConfig();
    engine.setCalibration(cal);
    engine.start();

    FrameData frame;
    frame.timestampMs = 1000;
    engine.pushFrame(frame);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    PhysicsState state = engine.getLatestState();
    REQUIRE_FALSE(state.tracking);

    engine.stop();
}

TEST_CASE("PhysicsEngine detects tracking when hand present", "[PhysicsEngine]")
{
    PhysicsEngine engine;
    CalibrationData cal = CalibrationData::defaultConfig();
    engine.setCalibration(cal);
    engine.start();

    FrameData frame;
    frame.timestampMs = 1000;
    HandLandmarks hand;
    hand.landmarks.resize(21);
    for (auto& lm : hand.landmarks)
        lm = {0.5f, 0.5f, 0.0f, 0.9f, 0.9f};
    hand.isLeft = true;
    hand.palmBox.confidence = 0.9f;
    frame.hands.push_back(hand);

    engine.pushFrame(frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    PhysicsState state = engine.getLatestState();
    REQUIRE(state.tracking);

    engine.stop();
}

TEST_CASE("PhysicsEngine generates note events on strum", "[PhysicsEngine]")
{
    PhysicsEngine engine;
    CalibrationData cal = CalibrationData::defaultConfig();
    engine.setCalibration(cal);

    std::vector<NoteEvent> received;
    engine.setNoteCallback([&received](const NoteEvent& evt) {
        received.push_back(evt);
    });

    engine.start();

    FrameData frame;
    frame.timestampMs = 1000;

    HandLandmarks fretHand;
    fretHand.landmarks.resize(21);
    for (auto& lm : fretHand.landmarks)
        lm = {0.3f, 0.5f, 0.0f, 0.9f, 0.9f};
    fretHand.isLeft = true;
    fretHand.palmBox.confidence = 0.9f;

    HandLandmarks strumHand;
    strumHand.landmarks.resize(21);
    for (auto& lm : strumHand.landmarks)
        lm = {0.82f, cal.strumZoneTop, 0.0f, 0.9f, 0.9f};
    strumHand.isLeft = false;
    strumHand.palmBox.confidence = 0.9f;

    frame.hands.push_back(fretHand);
    frame.hands.push_back(strumHand);
    engine.pushFrame(frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    frame.timestampMs = 1010;
    for (auto& lm : strumHand.landmarks)
        lm.y = cal.stringBottomY + 0.05f;
    frame.hands[1] = strumHand;
    engine.pushFrame(frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    engine.stop();

    bool hasNoteOn = false;
    for (auto& e : received)
    {
        if (e.type == NoteEventType::NoteOn)
            hasNoteOn = true;
    }
    REQUIRE(hasNoteOn);
}

TEST_CASE("PhysicsEngine updates chord name", "[PhysicsEngine]")
{
    PhysicsEngine engine;
    CalibrationData cal = CalibrationData::defaultConfig();
    engine.setCalibration(cal);
    engine.start();

    FrameData frame;
    frame.timestampMs = 1000;

    HandLandmarks fretHand;
    fretHand.landmarks.resize(21);
    float cx = 0.3f, cy = 0.5f;
    fretHand.landmarks[0] = {cx, cy, 0.0f, 0.9f, 0.9f};
    for (int f = 0; f < 4; ++f)
    {
        int base = 5 + f * 4;
        float fx = cx + static_cast<float>(f) * 0.01f;
        fretHand.landmarks[base]     = {fx, cy - 0.01f, 0.0f, 0.9f, 0.9f};
        fretHand.landmarks[base + 1] = {fx, cy - 0.02f, 0.0f, 0.9f, 0.9f};
        fretHand.landmarks[base + 2] = {fx, cy - 0.03f, 0.0f, 0.9f, 0.9f};
        fretHand.landmarks[base + 3] = {fx, cy - 0.04f, 0.0f, 0.9f, 0.9f};
    }
    fretHand.landmarks[1] = {cx - 0.04f, cy - 0.02f, 0.0f, 0.9f, 0.9f};
    fretHand.landmarks[2] = {cx - 0.03f, cy - 0.01f, 0.0f, 0.9f, 0.9f};
    fretHand.landmarks[3] = {cx - 0.04f, cy + 0.01f, 0.0f, 0.9f, 0.9f};
    fretHand.landmarks[4] = {cx - 0.09f, cy, 0.0f, 0.9f, 0.9f};

    fretHand.isLeft = true;
    fretHand.palmBox.confidence = 0.9f;

    frame.hands.push_back(fretHand);
    engine.pushFrame(frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    PhysicsState state = engine.getLatestState();
    REQUIRE(state.tracking);
    REQUIRE_FALSE(state.chordName.empty());

    engine.stop();
}
