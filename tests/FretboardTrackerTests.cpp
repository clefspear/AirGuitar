#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Physics/FretboardTracker.h"
#include "Physics/CalibrationData.h"

using namespace AirGuitar;
using Catch::Approx;

static HandLandmarks makeHand(const std::vector<Landmark>& landmarks, bool isLeft = true)
{
    HandLandmarks h;
    h.landmarks = landmarks;
    h.isLeft = isLeft;
    h.palmBox.confidence = 0.9f;
    h.timestampMs = 1000;
    return h;
}

static Landmark lm(float x, float y, float z = 0.0f)
{
    Landmark l;
    l.x = x;
    l.y = y;
    l.z = z;
    l.visibility = 0.9f;
    l.presence = 0.9f;
    return l;
}

static std::vector<Landmark> makeOpenHand(float cx, float cy)
{
    std::vector<Landmark> lms(21);
    lms[0] = lm(cx - 0.02f, cy);

    lms[1] = lm(cx - 0.04f, cy - 0.02f);
    lms[2] = lm(cx - 0.03f, cy - 0.01f);
    lms[3] = lm(cx - 0.04f, cy + 0.01f);
    lms[4] = lm(cx - 0.05f, cy + 0.02f);

    for (int f = 0; f < 4; ++f)
    {
        int base = 5 + f * 4;
        float fingerX = cx + static_cast<float>(f) * 0.01f;
        lms[base]     = lm(fingerX, cy - 0.01f);
        lms[base + 1] = lm(fingerX + 0.005f, cy - 0.025f);
        lms[base + 2] = lm(fingerX + 0.005f, cy - 0.035f);
        lms[base + 3] = lm(fingerX + 0.005f, cy - 0.045f);
    }

    return lms;
}

TEST_CASE("FretboardTracker maps horizontal position to fret", "[FretboardTracker]")
{
    FretboardTracker tracker;
    CalibrationData cal = CalibrationData::defaultConfig();

    auto lms = makeOpenHand(cal.fretOriginX + cal.fretScaleX * 3.0f, 0.5f);
    auto hand = makeHand(lms);

    tracker.update(hand, cal);
    REQUIRE(tracker.getCurrentFret() == 3);
}

TEST_CASE("FretboardTracker clamps fret to 0-24 range", "[FretboardTracker]")
{
    FretboardTracker tracker;
    CalibrationData cal = CalibrationData::defaultConfig();

    auto lms1 = makeOpenHand(0.0f, 0.5f);
    auto hand1 = makeHand(lms1);
    tracker.update(hand1, cal);
    REQUIRE(tracker.getCurrentFret() >= 0);

    tracker.reset();

    auto lms2 = makeOpenHand(1.0f, 0.5f);
    auto hand2 = makeHand(lms2);
    tracker.update(hand2, cal);
    REQUIRE(tracker.getCurrentFret() <= 24);
}

TEST_CASE("FretboardTracker detects open hand as no active strings", "[FretboardTracker]")
{
    FretboardTracker tracker;
    CalibrationData cal = CalibrationData::defaultConfig();

    auto lms = makeOpenHand(0.3f, 0.1f);
    auto hand = makeHand(lms);

    FretState state = tracker.update(hand, cal);
    bool anyActive = false;
    for (int i = 0; i < 6; ++i)
    {
        if (state.activeStrings[i] != -1)
            anyActive = true;
    }
    REQUIRE_FALSE(anyActive);
}

TEST_CASE("FretboardTracker detects barre chord when thumb extends", "[FretboardTracker]")
{
    FretboardTracker tracker;
    CalibrationData cal = CalibrationData::defaultConfig();

    auto lms = makeOpenHand(0.3f, 0.5f);
    lms[4] = lm(0.22f, 0.5f);

    auto hand = makeHand(lms);
    FretState state = tracker.update(hand, cal);

    REQUIRE(tracker.isBarre());
    for (int i = 0; i < 6; ++i)
    {
        REQUIRE(state.activeStrings[i] != -1);
    }
}

TEST_CASE("FretboardTracker reset clears state", "[FretboardTracker]")
{
    FretboardTracker tracker;
    CalibrationData cal = CalibrationData::defaultConfig();

    auto lms = makeOpenHand(0.3f, 0.5f);
    auto hand = makeHand(lms);
    tracker.update(hand, cal);
    REQUIRE(tracker.getCurrentFret() > 0);

    tracker.reset();
    REQUIRE(tracker.getCurrentFret() == 0);
}

TEST_CASE("FretboardTracker handles invalid hand gracefully", "[FretboardTracker]")
{
    FretboardTracker tracker;
    CalibrationData cal = CalibrationData::defaultConfig();

    HandLandmarks invalid;
    invalid.palmBox.confidence = 0.0f;

    FretState state = tracker.update(invalid, cal);
    REQUIRE(state.fret == 0);
}

TEST_CASE("FretboardTracker smooths fret transitions", "[FretboardTracker]")
{
    FretboardTracker tracker;
    CalibrationData cal = CalibrationData::defaultConfig();

    auto lms1 = makeOpenHand(cal.fretOriginX + cal.fretScaleX * 2.0f, 0.5f);
    auto hand1 = makeHand(lms1);
    for (int i = 0; i < 50; ++i)
        tracker.update(hand1, cal);
    int fret1 = tracker.getCurrentFret();

    auto lms2 = makeOpenHand(cal.fretOriginX + cal.fretScaleX * 8.0f, 0.5f);
    auto hand2 = makeHand(lms2);
    for (int i = 0; i < 50; ++i)
        tracker.update(hand2, cal);
    int fret2 = tracker.getCurrentFret();

    REQUIRE(fret2 > fret1);
}
