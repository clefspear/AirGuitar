#include "StrumDetector.h"
#include <algorithm>
#include <cmath>

namespace AirGuitar {

StrumDetector::StrumDetector()
    : xFilter(10.0f, 0.05f, 1.0f)
    , yFilter(10.0f, 0.05f, 1.0f)
{
}

std::vector<NoteEvent> StrumDetector::update(const HandLandmarks& hand,
                                              const CalibrationData& cal,
                                              int currentFret,
                                              const int activeStrings[6],
                                              int64_t timestampMs)
{
    std::vector<NoteEvent> events;

    if (!hand.valid() || hand.landmarks.size() < 21)
    {
        prevInZone = inZone;
        inZone = false;
        return events;
    }

    float rawX = hand.landmarks[0].x;
    float rawY = hand.landmarks[0].y;

    float dt = 0.004167f;
    if (prevTimestampMs > 0 && timestampMs > prevTimestampMs)
        dt = static_cast<float>(timestampMs - prevTimestampMs) / 1000.0f;

    float filteredX = xFilter.filter(rawX, dt);
    float filteredY = yFilter.filter(rawY, dt);

    prevInZone = inZone;
    inZone = filteredX >= cal.strumZoneLeft && filteredX <= cal.strumZoneRight
          && filteredY >= cal.strumZoneTop && filteredY <= cal.strumZoneBottom;

    if (prevTimestampMs > 0 && prevX >= 0.0f && prevY >= 0.0f && inZone)
    {
        float vx = filteredX - prevX;
        float vy = filteredY - prevY;
        float speed = std::sqrt(vx * vx + vy * vy);

        if (speed >= cal.minStrumSpeed)
        {
            float velocity = std::min(1.0f, speed / cal.maxStrumSpeed);
            velocity = std::max(0.0f, velocity);

            StrumDirection dir = (vy > 0.0f) ? StrumDirection::Down : StrumDirection::Up;

            auto crossings = interpolateCrossings(prevY, filteredY, cal);

            for (float crossY : crossings)
            {
                for (int s = 0; s < cal.kNumStrings; ++s)
                {
                    float stringY = computeStringY(s, cal);
                    float diff = std::abs(crossY - stringY);
                    float tolerance = std::abs(filteredY - prevY) * 0.5f + 0.01f;

                    if (diff < tolerance)
                    {
                        int64_t elapsed = timestampMs - lastStrumTimeMs[s];
                        if (elapsed >= cal.strumCooldownMs || lastStrumTimeMs[s] < 0)
                        {
                            int midiNote = cal.midiNoteForString(s, currentFret);

                            NoteEvent noteOn;
                            noteOn.type = NoteEventType::NoteOn;
                            noteOn.midiNote = midiNote;
                            noteOn.stringIndex = s;
                            noteOn.fret = currentFret;
                            noteOn.velocity = velocity;
                            noteOn.direction = dir;
                            noteOn.timestampMs = timestampMs;
                            events.push_back(noteOn);

                            lastStrumTimeMs[s] = timestampMs;
                            lastVelocity = velocity;
                            lastDirection = dir;
                            previousFretForString[s] = currentFret;
                        }
                    }
                }
            }
        }
    }

    prevX = filteredX;
    prevY = filteredY;
    prevTimestampMs = timestampMs;

    if (prevInZone && !inZone)
    {
        for (int s = 0; s < 6; ++s)
        {
            if (previousActiveStrings[s] >= 0)
            {
                NoteEvent noteOff;
                noteOff.type = NoteEventType::NoteOff;
                noteOff.midiNote = previousActiveStrings[s];
                noteOff.stringIndex = s;
                noteOff.fret = previousFretForString[s];
                noteOff.velocity = 0.0f;
                noteOff.direction = lastDirection;
                noteOff.timestampMs = timestampMs;
                events.push_back(noteOff);
                previousActiveStrings[s] = -1;
            }
        }
    }

    for (auto& evt : events)
    {
        if (evt.type == NoteEventType::NoteOn)
            previousActiveStrings[evt.stringIndex] = evt.midiNote;
    }

    return events;
}

void StrumDetector::reset()
{
    prevX = -1.0f;
    prevY = -1.0f;
    prevTimestampMs = 0;
    inZone = false;
    prevInZone = false;
    lastVelocity = 0.0f;
    lastDirection = StrumDirection::None;
    lastStrumTimeMs = {-1, -1, -1, -1, -1, -1};
    previousActiveStrings = {-1, -1, -1, -1, -1, -1};
    previousFretForString = {0, 0, 0, 0, 0, 0};
    xFilter.reset(0.0f);
    yFilter.reset(0.0f);
}

float StrumDetector::computeStringY(int stringIndex, const CalibrationData& cal)
{
    if (cal.kNumStrings <= 1)
        return cal.stringTopY;
    float t = static_cast<float>(stringIndex) / static_cast<float>(cal.kNumStrings - 1);
    return cal.stringTopY + t * (cal.stringBottomY - cal.stringTopY);
}

bool StrumDetector::crossedString(float prevY, float currY, float stringY, StrumDirection& dir)
{
    if (prevY < stringY && currY >= stringY)
    {
        dir = StrumDirection::Down;
        return true;
    }
    if (prevY > stringY && currY <= stringY)
    {
        dir = StrumDirection::Up;
        return true;
    }
    return false;
}

std::vector<float> StrumDetector::interpolateCrossings(float prevY, float currY, const CalibrationData& cal)
{
    std::vector<float> crossings;

    if (std::abs(currY - prevY) < 0.001f)
        return crossings;

    float minY = std::min(prevY, currY);
    float maxY = std::max(prevY, currY);

    for (int s = 0; s < cal.kNumStrings; ++s)
    {
        float stringY = computeStringY(s, cal);
        if (stringY >= minY && stringY <= maxY)
        {
            crossings.push_back(stringY);
        }
    }

    std::sort(crossings.begin(), crossings.end());

    if (currY < prevY)
        std::reverse(crossings.begin(), crossings.end());

    return crossings;
}

} // namespace AirGuitar
