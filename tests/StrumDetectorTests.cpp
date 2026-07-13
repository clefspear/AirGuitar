#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Physics/StrumDetector.h"
#include "Physics/CalibrationData.h"

using namespace AirGuitar;
using Catch::Approx;

static HandLandmarks makeStrumHand(float x, float y)
{
    HandLandmarks h;
    h.landmarks.resize(21);
    h.landmarks[0] = {x, y, 0.0f, 0.9f, 0.9f};
    h.isLeft = false;
    h.palmBox.confidence = 0.9f;
    h.timestampMs = 1000;
    return h;
}

TEST_CASE("StrumDetector detects downward strum", "[StrumDetector]")
{
    StrumDetector detector;
    CalibrationData cal = CalibrationData::defaultConfig();
    int activeStrings[6] = {-1, -1, -1, -1, -1, -1};

    float strumX = 0.82f;
    detector.update(makeStrumHand(strumX, cal.strumZoneTop + 0.05f), cal, 0, activeStrings, 1000);
    detector.update(makeStrumHand(strumX, cal.stringTopY - 0.02f), cal, 0, activeStrings, 1004);

    auto events = detector.update(makeStrumHand(strumX, cal.stringBottomY + 0.02f), cal, 0, activeStrings, 1008);

    bool hasNoteOn = false;
    for (auto& e : events)
    {
        if (e.type == NoteEventType::NoteOn)
            hasNoteOn = true;
    }
    REQUIRE(hasNoteOn);
}

TEST_CASE("StrumDetector detects upward strum", "[StrumDetector]")
{
    StrumDetector detector;
    CalibrationData cal = CalibrationData::defaultConfig();
    int activeStrings[6] = {-1, -1, -1, -1, -1, -1};

    float strumX = 0.82f;
    detector.update(makeStrumHand(strumX, cal.strumZoneBottom - 0.05f), cal, 0, activeStrings, 1000);
    detector.update(makeStrumHand(strumX, cal.stringBottomY + 0.02f), cal, 0, activeStrings, 1004);

    auto events = detector.update(makeStrumHand(strumX, cal.stringTopY - 0.02f), cal, 0, activeStrings, 1008);

    bool hasUpStrum = false;
    for (auto& e : events)
    {
        if (e.type == NoteEventType::NoteOn && e.direction == StrumDirection::Up)
            hasUpStrum = true;
    }
    REQUIRE(hasUpStrum);
}

TEST_CASE("StrumDetector ignores slow movement", "[StrumDetector]")
{
    StrumDetector detector;
    CalibrationData cal = CalibrationData::defaultConfig();
    int activeStrings[6] = {-1, -1, -1, -1, -1, -1};

    float strumX = 0.82f;
    float y = cal.stringTopY;
    detector.update(makeStrumHand(strumX, y), cal, 0, activeStrings, 1000);

    y += 0.0001f;
    auto events = detector.update(makeStrumHand(strumX, y), cal, 0, activeStrings, 1004);

    REQUIRE(events.empty());
}

TEST_CASE("StrumDetector respects per-string cooldown", "[StrumDetector]")
{
    StrumDetector detector;
    CalibrationData cal = CalibrationData::defaultConfig();
    cal.strumCooldownMs = 200;
    int activeStrings[6] = {-1, -1, -1, -1, -1, -1};

    float strumX = 0.82f;

    detector.update(makeStrumHand(strumX, cal.strumZoneTop), cal, 0, activeStrings, 1000);
    detector.update(makeStrumHand(strumX, cal.stringBottomY + 0.02f), cal, 0, activeStrings, 1004);

    auto events1 = detector.update(makeStrumHand(strumX, cal.stringBottomY + 0.05f), cal, 0, activeStrings, 1008);
    int count1 = 0;
    for (auto& e : events1)
        if (e.type == NoteEventType::NoteOn) count1++;

    auto events2 = detector.update(makeStrumHand(strumX, cal.stringBottomY + 0.1f), cal, 0, activeStrings, 1012);
    int count2 = 0;
    for (auto& e : events2)
        if (e.type == NoteEventType::NoteOn) count2++;

    REQUIRE(count2 <= count1);
}

TEST_CASE("StrumDetector ignores hand outside strum zone", "[StrumDetector]")
{
    StrumDetector detector;
    CalibrationData cal = CalibrationData::defaultConfig();
    int activeStrings[6] = {-1, -1, -1, -1, -1, -1};

    auto events1 = detector.update(makeStrumHand(0.2f, 0.5f), cal, 0, activeStrings, 1000);
    REQUIRE_FALSE(detector.isInStrumZone());

    auto events2 = detector.update(makeStrumHand(0.2f, 0.8f), cal, 0, activeStrings, 1004);
    REQUIRE(events2.empty());
}

TEST_CASE("StrumDetector reset clears state", "[StrumDetector]")
{
    StrumDetector detector;
    CalibrationData cal = CalibrationData::defaultConfig();
    int activeStrings[6] = {-1, -1, -1, -1, -1, -1};

    detector.update(makeStrumHand(0.82f, cal.strumZoneTop), cal, 0, activeStrings, 1000);
    detector.update(makeStrumHand(0.82f, cal.stringBottomY + 0.02f), cal, 0, activeStrings, 1004);

    detector.reset();
    REQUIRE_FALSE(detector.isInStrumZone());
    REQUIRE(detector.getLastVelocity() == Approx(0.0f));
}

TEST_CASE("StrumDetector uses correct MIDI notes", "[StrumDetector]")
{
    StrumDetector detector;
    CalibrationData cal = CalibrationData::defaultConfig();
    int activeStrings[6] = {-1, -1, -1, -1, -1, -1};

    float strumX = 0.82f;
    detector.update(makeStrumHand(strumX, cal.strumZoneTop), cal, 0, activeStrings, 1000);
    detector.update(makeStrumHand(strumX, cal.stringBottomY + 0.02f), cal, 0, activeStrings, 1004);

    auto events = detector.update(makeStrumHand(strumX, cal.stringBottomY + 0.05f), cal, 2, activeStrings, 1008);

    for (auto& e : events)
    {
        if (e.type == NoteEventType::NoteOn)
        {
            int expected = cal.openStringNotes[e.stringIndex] + 2;
            REQUIRE(e.midiNote == expected);
        }
    }
}

TEST_CASE("StrumDetector handles invalid hand gracefully", "[StrumDetector]")
{
    StrumDetector detector;
    CalibrationData cal = CalibrationData::defaultConfig();
    int activeStrings[6] = {-1, -1, -1, -1, -1, -1};

    HandLandmarks invalid;
    invalid.palmBox.confidence = 0.0f;

    auto events = detector.update(invalid, cal, 0, activeStrings, 1000);
    REQUIRE(events.empty());
}
