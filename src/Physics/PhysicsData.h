#pragma once
#include "NoteEvent.h"
#include "CalibrationData.h"
#include "OneEuroFilter.h"
#include "Vision/LandmarkData.h"
#include <array>

namespace AirGuitar {

struct FilteredLandmarks
{
    std::array<Landmark, 21> landmarks;
    bool isLeft = false;
    float confidence = 0.0f;
    int64_t timestampMs = 0;
};

struct HandAssignment
{
    const HandLandmarks* fretHand = nullptr;
    const HandLandmarks* strumHand = nullptr;
};

} // namespace AirGuitar
