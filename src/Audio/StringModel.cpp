#include "StringModel.h"
#include <cmath>

namespace AirGuitar {

StringModel::StringModel() {}

void StringModel::init(int sr)
{
    sampleRate = sr;
    for (int i = 0; i < 6; ++i)
    {
        int note = CalibrationData::openStringNotes[i];
        float freq = 440.0f * std::pow(2.0f, static_cast<float>(note - 69) / 12.0f);
        strings[i].init(freq, sampleRate);
        active[i] = false;
    }
}

void StringModel::noteOn(int stringIndex, int fret, float velocity, const CalibrationData& cal)
{
    if (stringIndex < 0 || stringIndex >= 6)
        return;

    int midiNote = cal.midiNoteForString(stringIndex, fret);
    float freq = 440.0f * std::pow(2.0f, static_cast<float>(midiNote - 69) / 12.0f);

    strings[stringIndex].init(freq, sampleRate);
    strings[stringIndex].pluck(velocity);
    active[stringIndex] = true;
}

void StringModel::noteOff(int stringIndex)
{
    if (stringIndex < 0 || stringIndex >= 6)
        return;

    strings[stringIndex].setDecay(0.98f);
}

float StringModel::mix()
{
    float output = 0.0f;
    for (int i = 0; i < 6; ++i)
    {
        if (active[i])
        {
            output += strings[i].nextSample();
            if (!strings[i].isActive())
                active[i] = false;
        }
    }
    return output * masterVolume / 6.0f;
}

void StringModel::setMasterVolume(float vol)
{
    masterVolume = std::max(0.0f, std::min(1.0f, vol));
}

bool StringModel::isAnyActive() const
{
    for (int i = 0; i < 6; ++i)
    {
        if (active[i])
            return true;
    }
    return false;
}

void StringModel::reset()
{
    for (int i = 0; i < 6; ++i)
    {
        strings[i].reset();
        active[i] = false;
    }
}

} // namespace AirGuitar
