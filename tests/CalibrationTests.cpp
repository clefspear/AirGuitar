#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Calibration/CalibrationManager.h"
#include <cstdio>

using namespace AirGuitar;
using Catch::Approx;

TEST_CASE("CalibrationManager saves and loads correctly", "[CalibrationManager]")
{
    CalibrationManager mgr;
    CalibrationData original = CalibrationData::defaultConfig();

    std::string testPath = "/tmp/airguitar_test_calibration";

    REQUIRE(mgr.save(original, testPath));

    CalibrationData loaded;
    REQUIRE(mgr.load(loaded, testPath));

    REQUIRE(loaded.fretOriginX == Approx(original.fretOriginX));
    REQUIRE(loaded.fretScaleX == Approx(original.fretScaleX));
    REQUIRE(loaded.fret1X == Approx(original.fret1X));
    REQUIRE(loaded.fret12X == Approx(original.fret12X));
    REQUIRE(loaded.stringTopY == Approx(original.stringTopY));
    REQUIRE(loaded.stringBottomY == Approx(original.stringBottomY));
    REQUIRE(loaded.strumZoneLeft == Approx(original.strumZoneLeft));
    REQUIRE(loaded.strumZoneRight == Approx(original.strumZoneRight));
    REQUIRE(loaded.leftHandFretting == original.leftHandFretting);
    REQUIRE(loaded.funMode == original.funMode);
    REQUIRE(loaded.strumCooldownMs == original.strumCooldownMs);

    std::remove(testPath.c_str());
}

TEST_CASE("CalibrationManager returns false for missing file", "[CalibrationManager]")
{
    CalibrationManager mgr;
    CalibrationData loaded;
    REQUIRE_FALSE(mgr.load(loaded, "/tmp/nonexistent_calibration_file_12345"));
}

TEST_CASE("CalibrationManager default path is valid", "[CalibrationManager]")
{
    CalibrationManager mgr;
    std::string path = mgr.getDefaultPath();
    REQUIRE_FALSE(path.empty());
}

TEST_CASE("CalibrationManager preserves fun mode chords", "[CalibrationManager]")
{
    CalibrationManager mgr;
    CalibrationData original = CalibrationData::defaultConfig();
    original.funModeChords = {48, 53, 55, 52, 57, 59};

    std::string testPath = "/tmp/airguitar_test_fun_chords";
    mgr.save(original, testPath);

    CalibrationData loaded;
    mgr.load(loaded, testPath);

    REQUIRE(loaded.funModeChords[0] == 48);
    REQUIRE(loaded.funModeChords[1] == 53);
    REQUIRE(loaded.funModeChords[2] == 55);
    REQUIRE(loaded.funModeChords[3] == 52);
    REQUIRE(loaded.funModeChords[4] == 57);
    REQUIRE(loaded.funModeChords[5] == 59);

    std::remove(testPath.c_str());
}

TEST_CASE("CalibrationData default strum zone is on right side for mirrored camera",
          "[CalibrationData][Mirrored]")
{
    CalibrationData cal = CalibrationData::defaultConfig();

    REQUIRE(cal.strumZoneLeft > 0.5f);
    REQUIRE(cal.strumZoneRight > cal.strumZoneLeft);
    REQUIRE(cal.strumZoneRight <= 1.0f);
    REQUIRE(cal.strumZoneTop < cal.strumZoneBottom);
}

TEST_CASE("CalibrationData default fret geometry is on left side",
          "[CalibrationData]")
{
    CalibrationData cal = CalibrationData::defaultConfig();

    REQUIRE(cal.fretOriginX < 0.5f);
    REQUIRE(cal.fret1X < cal.fret12X);
    REQUIRE(cal.fretScaleX > 0.0f);
}

TEST_CASE("CalibrationData midiNoteForString returns valid MIDI notes",
          "[CalibrationData]")
{
    CalibrationData cal = CalibrationData::defaultConfig();

    for (int s = 0; s < 6; ++s)
    {
        int openNote = cal.midiNoteForString(s, 0);
        REQUIRE(openNote > 0);
        REQUIRE(openNote < 128);

        int fretted = cal.midiNoteForString(s, 5);
        REQUIRE(fretted == openNote + 5);
    }
}

TEST_CASE("CalibrationData midiNoteForString rejects invalid string index",
          "[CalibrationData][EdgeCase]")
{
    CalibrationData cal = CalibrationData::defaultConfig();

    REQUIRE(cal.midiNoteForString(-1, 0) == -1);
    REQUIRE(cal.midiNoteForString(6, 0) == -1);
}

TEST_CASE("CalibrationData open strings span correct range",
          "[CalibrationData]")
{
    CalibrationData cal = CalibrationData::defaultConfig();

    REQUIRE(cal.openStringNotes[0] == 40);
    REQUIRE(cal.openStringNotes[5] == 64);

    for (int i = 0; i < 5; ++i)
        REQUIRE(cal.openStringNotes[i] < cal.openStringNotes[i + 1]);
}

TEST_CASE("CalibrationData default strum speed thresholds are reasonable",
          "[CalibrationData]")
{
    CalibrationData cal = CalibrationData::defaultConfig();

    REQUIRE(cal.minStrumSpeed > 0.0f);
    REQUIRE(cal.minStrumSpeed < cal.maxStrumSpeed);
    REQUIRE(cal.maxStrumSpeed < 1.0f);
    REQUIRE(cal.strumCooldownMs > 0);
    REQUIRE(cal.strumCooldownMs < 500);
}
