#pragma once
#include <array>
#include <string>

namespace AirGuitar {

struct CalibrationData
{
    float fretOriginX = 0.0f;
    float fretScaleX = 0.01f;
    float fret1X = 0.2f;
    float fret12X = 0.8f;

    float stringTopY = 0.2f;
    float stringBottomY = 0.8f;

    float strumZoneLeft = 0.7f;
    float strumZoneRight = 0.95f;
    float strumZoneTop = 0.15f;
    float strumZoneBottom = 0.85f;

    float extendedThreshold = 0.7f;
    float referenceHandSize = 0.15f;

    bool leftHandFretting = false;
    bool funMode = false;

    std::array<int, 6> funModeChords = {55, 60, 62, 52, 57, 52};

    float minStrumSpeed = 0.005f;
    float maxStrumSpeed = 0.08f;
    int strumCooldownMs = 50;

    static constexpr int kNumStrings = 6;
    static constexpr int kMaxFrets = 24;

    static constexpr std::array<int, 6> openStringNotes = {40, 45, 50, 55, 59, 64};

    int midiNoteForString(int stringIndex, int fret) const
    {
        if (stringIndex < 0 || stringIndex >= kNumStrings)
            return -1;
        return openStringNotes[stringIndex] + fret;
    }

    static CalibrationData defaultConfig()
    {
        CalibrationData c;
        c.fretOriginX = 0.15f;
        c.fretScaleX = 0.035f;
        c.fret1X = 0.15f;
        c.fret12X = 0.55f;
        c.stringTopY = 0.25f;
        c.stringBottomY = 0.75f;
        c.strumZoneLeft = 0.7f;
        c.strumZoneRight = 0.95f;
        c.strumZoneTop = 0.15f;
        c.strumZoneBottom = 0.85f;
        c.extendedThreshold = 0.7f;
        c.referenceHandSize = 0.15f;
        c.leftHandFretting = false;
        c.funMode = false;
        c.minStrumSpeed = 0.005f;
        c.maxStrumSpeed = 0.08f;
        c.strumCooldownMs = 50;
        return c;
    }
};

} // namespace AirGuitar
