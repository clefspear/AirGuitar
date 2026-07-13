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
