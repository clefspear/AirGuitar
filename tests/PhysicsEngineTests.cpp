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
    h.palmBox.width = 0.2f;
    h.palmBox.height = 0.2f;
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
    hand.palmBox.width = 0.2f;
    hand.palmBox.height = 0.2f;
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
    fretHand.palmBox.width = 0.2f;
    fretHand.palmBox.height = 0.2f;

    HandLandmarks strumHand;
    strumHand.landmarks.resize(21);
    for (auto& lm : strumHand.landmarks)
        lm = {0.82f, cal.strumZoneTop, 0.0f, 0.9f, 0.9f};
    strumHand.isLeft = false;
    strumHand.palmBox.confidence = 0.9f;
    strumHand.palmBox.width = 0.2f;
    strumHand.palmBox.height = 0.2f;

    frame.hands.push_back(fretHand);
    frame.hands.push_back(strumHand);
    engine.pushFrame(frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    frame.timestampMs = 1010;
    for (auto& lm : strumHand.landmarks)
        lm.y = cal.stringBottomY + 0.05f;
    frame.hands[1] = strumHand;
    engine.pushFrame(frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

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
    fretHand.palmBox.width = 0.2f;
    fretHand.palmBox.height = 0.2f;

    frame.hands.push_back(fretHand);
    engine.pushFrame(frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    PhysicsState state = engine.getLatestState();
    REQUIRE(state.tracking);
    REQUIRE_FALSE(state.chordName.empty());

    engine.stop();
}

TEST_CASE("PhysicsEngine preserves fret across short detection gap",
          "[PhysicsEngine][FretPreservation]")
{
    PhysicsEngine engine;
    CalibrationData cal = CalibrationData::defaultConfig();
    engine.setCalibration(cal);
    engine.start();

    FrameData frame;
    frame.timestampMs = 1000;

    HandLandmarks fretHand;
    fretHand.landmarks.resize(21);
    float handX = cal.fretOriginX + cal.fretScaleX * 5.0f;
    fretHand.landmarks[0] = {handX, 0.5f, 0.0f, 0.9f, 0.9f};
    for (int i = 1; i < 21; ++i)
        fretHand.landmarks[i] = {handX, 0.5f, 0.0f, 0.9f, 0.9f};
    fretHand.isLeft = true;
    fretHand.palmBox.confidence = 0.9f;
    fretHand.palmBox.width = 0.2f;
    fretHand.palmBox.height = 0.2f;

    frame.hands.push_back(fretHand);
    engine.pushFrame(frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    PhysicsState state1 = engine.getLatestState();
    REQUIRE(state1.tracking);
    int fretBefore = state1.fretState.fret;

    frame.hands.clear();
    frame.timestampMs = 1050;
    engine.pushFrame(frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    PhysicsState state2 = engine.getLatestState();
    int fretDuringGap = state2.fretState.fret;
    REQUIRE(fretDuringGap == fretBefore);

    engine.stop();
}

TEST_CASE("PhysicsEngine handles single strum hand with preserved fret",
          "[PhysicsEngine][SingleHand]")
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
    float handX = cal.fretOriginX + cal.fretScaleX * 3.0f;
    fretHand.landmarks[0] = {handX, 0.5f, 0.0f, 0.9f, 0.9f};
    for (int i = 1; i < 21; ++i)
        fretHand.landmarks[i] = {handX, 0.5f, 0.0f, 0.9f, 0.9f};
    fretHand.isLeft = true;
    fretHand.palmBox.confidence = 0.9f;
    fretHand.palmBox.width = 0.2f;
    fretHand.palmBox.height = 0.2f;

    frame.hands.push_back(fretHand);
    engine.pushFrame(frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    PhysicsState state1 = engine.getLatestState();
    int fretBefore = state1.fretState.fret;

    frame.hands.clear();
    frame.timestampMs = 1100;

    HandLandmarks strumHand;
    strumHand.landmarks.resize(21);
    for (auto& lm : strumHand.landmarks)
        lm = {0.82f, cal.strumZoneTop, 0.0f, 0.9f, 0.9f};
    strumHand.isLeft = false;
    strumHand.palmBox.confidence = 0.9f;
    strumHand.palmBox.width = 0.2f;
    strumHand.palmBox.height = 0.2f;

    frame.hands.push_back(strumHand);
    engine.pushFrame(frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    PhysicsState state2 = engine.getLatestState();
    REQUIRE(state2.fretState.fret == fretBefore);

    engine.stop();
}

TEST_CASE("PhysicsEngine single right hand generates strum events",
          "[PhysicsEngine][SingleHand][Strum]")
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

    HandLandmarks rightHand;
    rightHand.landmarks.resize(21);
    float strumX = 0.82f;
    for (auto& lm : rightHand.landmarks)
        lm = {strumX, cal.strumZoneTop + 0.02f, 0.0f, 0.9f, 0.9f};
    rightHand.isLeft = false;
    rightHand.palmBox.confidence = 0.9f;
    rightHand.palmBox.width = 0.2f;
    rightHand.palmBox.height = 0.2f;

    frame.hands.push_back(rightHand);
    engine.pushFrame(frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    frame.timestampMs = 1004;
    for (auto& lm : rightHand.landmarks)
        lm.y = cal.stringBottomY + 0.02f;
    frame.hands[0] = rightHand;
    engine.pushFrame(frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    engine.stop();

    bool hasNoteOn = false;
    for (auto& e : received)
    {
        if (e.type == NoteEventType::NoteOn)
            hasNoteOn = true;
    }
    REQUIRE(hasNoteOn);
}

TEST_CASE("PhysicsEngine single left hand does NOT generate strum events",
          "[PhysicsEngine][SingleHand][NoStrum]")
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
        lm = {0.3f, cal.strumZoneTop + 0.02f, 0.0f, 0.9f, 0.9f};
    fretHand.isLeft = true;
    fretHand.palmBox.confidence = 0.9f;
    fretHand.palmBox.width = 0.2f;
    fretHand.palmBox.height = 0.2f;

    frame.hands.push_back(fretHand);
    engine.pushFrame(frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    frame.timestampMs = 1004;
    for (auto& lm : fretHand.landmarks)
        lm.y = cal.stringBottomY + 0.02f;
    frame.hands[0] = fretHand;
    engine.pushFrame(frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    engine.stop();

    bool hasNoteOn = false;
    for (auto& e : received)
    {
        if (e.type == NoteEventType::NoteOn)
            hasNoteOn = true;
    }
    REQUIRE_FALSE(hasNoteOn);
}

TEST_CASE("PhysicsEngine leftHandFretting=false swaps hand roles",
          "[PhysicsEngine][SingleHand][LeftStrum]")
{
    PhysicsEngine engine;
    CalibrationData cal = CalibrationData::defaultConfig();
    cal.leftHandFretting = false;
    engine.setCalibration(cal);

    std::vector<NoteEvent> received;
    engine.setNoteCallback([&received](const NoteEvent& evt) {
        received.push_back(evt);
    });

    engine.start();

    FrameData frame;
    frame.timestampMs = 1000;

    HandLandmarks leftHand;
    leftHand.landmarks.resize(21);
    float strumX = 0.82f;
    for (auto& lm : leftHand.landmarks)
        lm = {strumX, cal.strumZoneTop + 0.02f, 0.0f, 0.9f, 0.9f};
    leftHand.isLeft = true;
    leftHand.palmBox.confidence = 0.9f;
    leftHand.palmBox.width = 0.2f;
    leftHand.palmBox.height = 0.2f;

    frame.hands.push_back(leftHand);
    engine.pushFrame(frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    frame.timestampMs = 1004;
    for (auto& lm : leftHand.landmarks)
        lm.y = cal.stringBottomY + 0.02f;
    frame.hands[0] = leftHand;
    engine.pushFrame(frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    engine.stop();

    bool hasNoteOn = false;
    for (auto& e : received)
    {
        if (e.type == NoteEventType::NoteOn)
            hasNoteOn = true;
    }
    REQUIRE(hasNoteOn);
}
