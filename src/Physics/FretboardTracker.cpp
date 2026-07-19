#include "FretboardTracker.h"
#include <algorithm>
#include <cmath>

namespace AirGuitar {

FretboardTracker::FretboardTracker()
    : fretFilter(1.5f, 0.02f, 1.0f)
{
}

FretState FretboardTracker::update(const HandLandmarks& hand, const CalibrationData& cal)
{
    if (!hand.valid() || hand.landmarks.size() != 21)
        return currentState;

    float cx = handCenterX(hand);
    float rawFret = (cx - cal.fretOriginX) / cal.fretScaleX;
    rawFret = std::max(0.0f, std::min(rawFret, static_cast<float>(cal.kMaxFrets)));

    float dt = 0.004167f;
    float filteredFret = fretFilter.filter(rawFret, dt);
    currentState.fret = static_cast<int>(std::round(filteredFret));
    currentState.fret = std::max(0, std::min(currentState.fret, cal.kMaxFrets));

    barreChord = isThumbExtended(hand);

    bool extended[5] = {
        isThumbExtended(hand),
        isFingerExtended(hand, 5),
        isFingerExtended(hand, 9),
        isFingerExtended(hand, 13),
        isFingerExtended(hand, 17)
    };

    for (int i = 0; i < 5; ++i)
        currentState.fingerExtended[i] = extended[i];

    for (int i = 0; i < 6; ++i)
        currentState.activeStrings[i] = -1;

    static constexpr int fingerIndices[4] = {5, 9, 13, 17};

    for (int f = 0; f < 4; ++f)
    {
        if (!extended[f + 1])
            continue;

        int tipIdx = fingerIndices[f] + 3;
        float tipY = hand.landmarks[tipIdx].y;

        if (tipY < cal.stringTopY || tipY > cal.stringBottomY)
            continue;

        int stringIdx = mapToNearestString(tipY, cal.stringTopY, cal.stringBottomY, cal.kNumStrings);

        if (stringIdx >= 0 && stringIdx < cal.kNumStrings && currentState.activeStrings[stringIdx] == -1)
        {
            currentState.activeStrings[stringIdx] = currentState.fret;
        }
    }

    if (barreChord)
    {
        for (int i = 0; i < cal.kNumStrings; ++i)
        {
            if (currentState.activeStrings[i] == -1)
                currentState.activeStrings[i] = currentState.fret;
        }
    }

    return currentState;
}

void FretboardTracker::reset()
{
    currentState = FretState{};
    barreChord = false;
    fretFilter.reset(0.0f);
}

float FretboardTracker::distance(const Landmark& a, const Landmark& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

float FretboardTracker::handCenterX(const HandLandmarks& hand)
{
    float sum = 0.0f;
    int count = 0;
    static constexpr int mcpIndices[] = {0, 5, 9, 13, 17};
    for (int idx : mcpIndices)
    {
        sum += hand.landmarks[idx].x;
        ++count;
    }
    return sum / static_cast<float>(count);
}

float FretboardTracker::handCenterY(const HandLandmarks& hand)
{
    float sum = 0.0f;
    int count = 0;
    static constexpr int mcpIndices[] = {0, 5, 9, 13, 17};
    for (int idx : mcpIndices)
    {
        sum += hand.landmarks[idx].y;
        ++count;
    }
    return sum / static_cast<float>(count);
}

bool FretboardTracker::isFingerExtended(const HandLandmarks& hand, int fingerStart)
{
    if (fingerStart + 3 >= static_cast<int>(hand.landmarks.size()))
        return false;

    const auto& mcp = hand.landmarks[fingerStart];
    const auto& pip = hand.landmarks[fingerStart + 1];
    const auto& tip = hand.landmarks[fingerStart + 3];

    float tipDist = distance(tip, mcp);
    float pipDist = distance(pip, mcp);

    if (pipDist < 0.001f)
        return false;

    return (tipDist / pipDist) > 1.2f;
}

bool FretboardTracker::isThumbExtended(const HandLandmarks& hand)
{
    if (hand.landmarks.size() < 21)
        return false;

    const auto& thumbTip = hand.landmarks[4];
    const auto& indexMcp = hand.landmarks[5];

    float spread = distance(thumbTip, indexMcp);
    return spread > 0.15f;
}

int FretboardTracker::mapToNearestString(float y, float topY, float bottomY, int numStrings)
{
    if (numStrings <= 0)
        return -1;

    float normalized = (y - topY) / (bottomY - topY);
    normalized = std::max(0.0f, std::min(1.0f, normalized));

    int stringIdx = static_cast<int>(std::round(normalized * static_cast<float>(numStrings - 1)));
    return std::max(0, std::min(stringIdx, numStrings - 1));
}

} // namespace AirGuitar
