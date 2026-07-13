#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Audio/KarplusStrong.h"
#include "Audio/StringModel.h"
#include "Physics/CalibrationData.h"
#include <cmath>

using namespace AirGuitar;
using Catch::Approx;

TEST_CASE("KarplusStrong produces output after pluck", "[KarplusStrong]")
{
    KarplusStrong ks;
    ks.init(440.0f, 44100);
    ks.pluck(0.8f);

    float sample = ks.nextSample();
    REQUIRE(std::abs(sample) > 0.0f);
}

TEST_CASE("KarplusStrong decays over time", "[KarplusStrong]")
{
    KarplusStrong ks;
    ks.init(440.0f, 44100);
    ks.pluck(1.0f);

    float initial = 0.0f;
    for (int i = 0; i < 100; ++i)
        initial += std::abs(ks.nextSample());
    initial /= 100.0f;

    float later = 0.0f;
    for (int i = 0; i < 100; ++i)
        later += std::abs(ks.nextSample());
    later /= 100.0f;

    REQUIRE(later < initial);
}

TEST_CASE("KarplusStrong stops after enough samples", "[KarplusStrong]")
{
    KarplusStrong ks;
    ks.init(440.0f, 44100);
    ks.pluck(0.5f);

    for (int i = 0; i < 100000; ++i)
        ks.nextSample();

    REQUIRE_FALSE(ks.isActive());
}

TEST_CASE("KarplusStrong reset works", "[KarplusStrong]")
{
    KarplusStrong ks;
    ks.init(440.0f, 44100);
    ks.pluck(1.0f);
    REQUIRE(ks.isActive());

    ks.reset();
    REQUIRE_FALSE(ks.isActive());
    REQUIRE(ks.nextSample() == Approx(0.0f));
}

TEST_CASE("KarplusStrong setDecay affects duration", "[KarplusStrong]")
{
    KarplusStrong ks1;
    ks1.init(440.0f, 44100);
    ks1.setDecay(0.999f);
    ks1.pluck(1.0f);

    int count1 = 0;
    while (ks1.isActive() && count1 < 200000)
    {
        ks1.nextSample();
        ++count1;
    }

    KarplusStrong ks2;
    ks2.init(440.0f, 44100);
    ks2.setDecay(0.990f);
    ks2.pluck(1.0f);

    int count2 = 0;
    while (ks2.isActive() && count2 < 200000)
    {
        ks2.nextSample();
        ++count2;
    }

    REQUIRE(count1 > count2);
}

TEST_CASE("StringModel produces output", "[StringModel]")
{
    StringModel model;
    model.init(44100);

    CalibrationData cal = CalibrationData::defaultConfig();
    model.noteOn(0, 0, 0.8f, cal);

    float sample = model.mix();
    REQUIRE(std::abs(sample) > 0.0f);
}

TEST_CASE("StringModel mixes multiple strings", "[StringModel]")
{
    StringModel model;
    model.init(44100);

    CalibrationData cal = CalibrationData::defaultConfig();
    model.noteOn(0, 0, 0.8f, cal);
    model.noteOn(1, 0, 0.8f, cal);

    REQUIRE(model.isAnyActive());

    float sample = model.mix();
    REQUIRE(std::abs(sample) > 0.0f);
}

TEST_CASE("StringModel master volume works", "[StringModel]")
{
    StringModel model;
    model.init(44100);

    CalibrationData cal = CalibrationData::defaultConfig();
    model.noteOn(0, 0, 1.0f, cal);
    float loud = model.mix();

    model.reset();
    model.setMasterVolume(0.1f);
    model.noteOn(0, 0, 1.0f, cal);
    float quiet = model.mix();

    REQUIRE(std::abs(quiet) < std::abs(loud));
}
