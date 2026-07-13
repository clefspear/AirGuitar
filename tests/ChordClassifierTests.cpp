#include <catch2/catch_test_macros.hpp>
#include "Physics/ChordClassifier.h"

using namespace AirGuitar;

TEST_CASE("ChordClassifier identifies major chord", "[ChordClassifier]")
{
    ChordClassifier classifier;
    ChordMatch result = classifier.classifyFromNotes({60, 64, 67});
    REQUIRE(result.valid);
    REQUIRE(result.name == "C");
}

TEST_CASE("ChordClassifier identifies minor chord", "[ChordClassifier]")
{
    ChordClassifier classifier;
    ChordMatch result = classifier.classifyFromNotes({60, 63, 67});
    REQUIRE(result.valid);
    REQUIRE(result.name == "Cm");
}

TEST_CASE("ChordClassifier identifies dominant 7th", "[ChordClassifier]")
{
    ChordClassifier classifier;
    ChordMatch result = classifier.classifyFromNotes({60, 64, 67, 70});
    REQUIRE(result.valid);
    REQUIRE(result.name == "C7");
}

TEST_CASE("ChordClassifier identifies major 7th", "[ChordClassifier]")
{
    ChordClassifier classifier;
    ChordMatch result = classifier.classifyFromNotes({60, 64, 67, 71});
    REQUIRE(result.valid);
    REQUIRE(result.name == "Cmaj7");
}

TEST_CASE("ChordClassifier identifies minor 7th", "[ChordClassifier]")
{
    ChordClassifier classifier;
    ChordMatch result = classifier.classifyFromNotes({60, 63, 67, 70});
    REQUIRE(result.valid);
    REQUIRE(result.name == "Cm7");
}

TEST_CASE("ChordClassifier identifies diminished", "[ChordClassifier]")
{
    ChordClassifier classifier;
    ChordMatch result = classifier.classifyFromNotes({60, 63, 66});
    REQUIRE(result.valid);
    REQUIRE(result.name == "Cdim");
}

TEST_CASE("ChordClassifier identifies augmented", "[ChordClassifier]")
{
    ChordClassifier classifier;
    ChordMatch result = classifier.classifyFromNotes({60, 64, 68});
    REQUIRE(result.valid);
    REQUIRE(result.name == "Caug");
}

TEST_CASE("ChordClassifier identifies sus2", "[ChordClassifier]")
{
    ChordClassifier classifier;
    ChordMatch result = classifier.classifyFromNotes({60, 62, 67});
    REQUIRE(result.valid);
    REQUIRE(result.name == "Csus2");
}

TEST_CASE("ChordClassifier identifies sus4", "[ChordClassifier]")
{
    ChordClassifier classifier;
    ChordMatch result = classifier.classifyFromNotes({60, 65, 67});
    REQUIRE(result.valid);
    REQUIRE(result.name == "Csus4");
}

TEST_CASE("ChordClassifier identifies single note", "[ChordClassifier]")
{
    ChordClassifier classifier;
    ChordMatch result = classifier.classifyFromNotes({60});
    REQUIRE(result.valid);
    REQUIRE(result.name == "C");
}

TEST_CASE("ChordClassifier returns invalid for empty input", "[ChordClassifier]")
{
    ChordClassifier classifier;
    ChordMatch result = classifier.classifyFromNotes({});
    REQUIRE_FALSE(result.valid);
}

TEST_CASE("ChordClassifier handles octave equivalence", "[ChordClassifier]")
{
    ChordClassifier classifier;
    ChordMatch result = classifier.classifyFromNotes({48, 64, 67});
    REQUIRE(result.valid);
    REQUIRE(result.name == "C");
}

TEST_CASE("ChordClassifier identifies power chord", "[ChordClassifier]")
{
    ChordClassifier classifier;
    ChordMatch result = classifier.classifyFromNotes({60, 67});
    REQUIRE(result.valid);
    REQUIRE(result.name == "C5");
}

TEST_CASE("ChordClassifier identifies from active strings", "[ChordClassifier]")
{
    ChordClassifier classifier;
    CalibrationData cal = CalibrationData::defaultConfig();

    int active[6] = {-1, -1, -1, -1, -1, -1};
    active[0] = 3;
    active[1] = 2;
    active[2] = 0;

    ChordMatch result = classifier.classify(0, active, cal);
    REQUIRE(result.valid);
}

TEST_CASE("ChordClassifier noteName correctness", "[ChordClassifier]")
{
    REQUIRE(ChordClassifier::noteName(60) == "C");
    REQUIRE(ChordClassifier::noteName(61) == "C#");
    REQUIRE(ChordClassifier::noteName(62) == "D");
    REQUIRE(ChordClassifier::noteName(64) == "E");
    REQUIRE(ChordClassifier::noteName(67) == "G");
    REQUIRE(ChordClassifier::noteName(69) == "A");
}

TEST_CASE("ChordClassifier identifies dim7", "[ChordClassifier]")
{
    ChordClassifier classifier;
    ChordMatch result = classifier.classifyFromNotes({60, 63, 66, 69});
    REQUIRE(result.valid);
    REQUIRE(result.name == "Cdim7");
}

TEST_CASE("ChordClassifier identifies m7b5", "[ChordClassifier]")
{
    ChordClassifier classifier;
    ChordMatch result = classifier.classifyFromNotes({60, 63, 66, 70});
    REQUIRE(result.valid);
    REQUIRE(result.name == "Cm7b5");
}

TEST_CASE("ChordClassifier identifies 9th chord", "[ChordClassifier]")
{
    ChordClassifier classifier;
    ChordMatch result = classifier.classifyFromNotes({60, 64, 67, 70, 74});
    REQUIRE(result.valid);
    REQUIRE(result.name == "C9");
}

TEST_CASE("ChordClassifier identifies 6th chord", "[ChordClassifier]")
{
    ChordClassifier classifier;
    ChordMatch result = classifier.classifyFromNotes({60, 64, 67, 69});
    REQUIRE(result.valid);
    REQUIRE(result.name == "C6");
}
