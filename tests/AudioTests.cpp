#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Audio/KarplusStrong.h"
#include "Audio/StringModel.h"
#include "Audio/AudioEngine.h"
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

    for (int i = 0; i < 2000000; ++i)
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
    while (ks1.isActive() && count1 < 2000000)
    {
        ks1.nextSample();
        ++count1;
    }

    KarplusStrong ks2;
    ks2.init(440.0f, 44100);
    ks2.setDecay(0.990f);
    ks2.pluck(1.0f);

    int count2 = 0;
    while (ks2.isActive() && count2 < 2000000)
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

    model.noteOn(0, 40, 0.8f);

    float sample = model.mix();
    REQUIRE(std::abs(sample) > 0.0f);
}

TEST_CASE("StringModel mixes multiple strings", "[StringModel]")
{
    StringModel model;
    model.init(44100);

    model.noteOn(0, 40, 0.8f);
    model.noteOn(1, 45, 0.8f);

    REQUIRE(model.isAnyActive());

    float sample = model.mix();
    REQUIRE(std::abs(sample) > 0.0f);
}

TEST_CASE("StringModel master volume works", "[StringModel]")
{
    StringModel model;
    model.init(44100);

    model.noteOn(0, 40, 1.0f);
    float loud = model.mix();

    model.reset();
    model.setMasterVolume(0.1f);
    model.noteOn(0, 40, 1.0f);
    float quiet = model.mix();

    REQUIRE(std::abs(quiet) < std::abs(loud));
}

TEST_CASE("AudioEngine getStartError returns empty on construction",
          "[AudioEngine][Error]")
{
    AudioEngine engine;
    REQUIRE(engine.getStartError().empty());
}

TEST_CASE("AudioEngine handleNoteEvent queues without crashing",
          "[AudioEngine][Event]")
{
    AudioEngine engine;
    NoteEvent evt;
    evt.type = NoteEventType::NoteOn;
    evt.midiNote = 40;
    evt.stringIndex = 0;
    evt.fret = 0;
    evt.velocity = 0.8f;
    evt.timestampMs = 1000;
    engine.handleNoteEvent(evt);

    NoteEvent off;
    off.type = NoteEventType::NoteOff;
    off.stringIndex = 0;
    off.midiNote = -1;
    off.velocity = 0.0f;
    off.timestampMs = 1001;
    engine.handleNoteEvent(off);
}

TEST_CASE("StringModel noteOn rejects out-of-range string index",
          "[StringModel][EdgeCase]")
{
    StringModel model;
    model.init(44100);

    model.noteOn(-1, 40, 0.8f);
    model.noteOn(6, 40, 0.8f);
    REQUIRE_FALSE(model.isAnyActive());
}

TEST_CASE("StringModel noteOff rejects out-of-range string index",
          "[StringModel][EdgeCase]")
{
    StringModel model;
    model.init(44100);

    model.noteOn(0, 40, 0.8f);
    REQUIRE(model.isAnyActive());

    model.noteOff(-1);
    model.noteOff(6);
    REQUIRE(model.isAnyActive());
}

TEST_CASE("StringModel mix with no active strings returns near-zero",
          "[StringModel][Silence]")
{
    StringModel model;
    model.init(44100);

    float sample = model.mix();
    REQUIRE(std::abs(sample) < 0.001f);
}

TEST_CASE("StringModel re-pluck on same string restarts note",
          "[StringModel][RePluck]")
{
    StringModel model;
    model.init(44100);

    model.noteOn(0, 40, 0.8f);
    float sample1 = model.mix();

    model.noteOn(0, 40, 1.0f);
    float sample2 = model.mix();

    REQUIRE(std::abs(sample1) > 0.0f);
    REQUIRE(std::abs(sample2) > 0.0f);
}

TEST_CASE("StringModel all 6 strings can play simultaneously",
          "[StringModel][Polyphony]")
{
    StringModel model;
    model.init(44100);

    for (int i = 0; i < 6; ++i)
        model.noteOn(i, 40 + i * 5, 0.7f);

    REQUIRE(model.isAnyActive());

    float sample = model.mix();
    REQUIRE(std::abs(sample) > 0.0f);
}
