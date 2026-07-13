#pragma once
#include "PhysicsData.h"
#include "CalibrationData.h"
#include "OneEuroFilter.h"
#include "Vision/LandmarkData.h"
#include <vector>
#include <array>

namespace AirGuitar {

class StrumDetector
{
public:
    StrumDetector();

    std::vector<NoteEvent> update(const HandLandmarks& hand,
                                   const CalibrationData& cal,
                                   int currentFret,
                                   const int activeStrings[6],
                                   int64_t timestampMs);

    void reset();

    bool isInStrumZone() const { return inZone; }
    float getLastVelocity() const { return lastVelocity; }
    StrumDirection getLastDirection() const { return lastDirection; }

private:
    OneEuroFilter xFilter;
    OneEuroFilter yFilter;

    float prevX = -1.0f;
    float prevY = -1.0f;
    int64_t prevTimestampMs = 0;

    bool inZone = false;
    bool prevInZone = false;
    float lastVelocity = 0.0f;
    StrumDirection lastDirection = StrumDirection::None;

    std::array<int64_t, 6> lastStrumTimeMs = {-1, -1, -1, -1, -1, -1};

    static float computeStringY(int stringIndex, const CalibrationData& cal);
    static bool crossedString(float prevY, float currY, float stringY, StrumDirection& dir);
    static std::vector<float> interpolateCrossings(float prevY, float currY, const CalibrationData& cal);
};

} // namespace AirGuitar
