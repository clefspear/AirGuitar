#pragma once
#include "PhysicsData.h"
#include "CalibrationData.h"
#include "OneEuroFilter.h"
#include "Vision/LandmarkData.h"
#include <array>

namespace AirGuitar {

class FretboardTracker
{
public:
    FretboardTracker();

    FretState update(const HandLandmarks& hand, const CalibrationData& cal);

    void reset();

    int getCurrentFret() const { return currentState.fret; }
    const FretState& getState() const { return currentState; }
    bool isBarre() const { return barreChord; }

private:
    FretState currentState;
    bool barreChord = false;

    OneEuroFilter fretFilter;

    static float distance(const Landmark& a, const Landmark& b);
    static float handCenterX(const HandLandmarks& hand);
    static float handCenterY(const HandLandmarks& hand);
    static bool isFingerExtended(const HandLandmarks& hand, int fingerStart);
    static bool isThumbExtended(const HandLandmarks& hand);
    static int mapToNearestString(float y, float topY, float bottomY, int numStrings);
};

} // namespace AirGuitar
