#pragma once
#include "KarplusStrong.h"
#include "Physics/CalibrationData.h"
#include "Physics/NoteEvent.h"
#include <array>

namespace AirGuitar {

class StringModel
{
public:
    StringModel();

    void init(int sampleRate);
    void noteOn(int stringIndex, int fret, float velocity, const CalibrationData& cal);
    void noteOff(int stringIndex);
    float mix();
    void setMasterVolume(float vol);
    void reset();

    bool isAnyActive() const;

private:
    std::array<KarplusStrong, 6> strings;
    std::array<bool, 6> active{};
    float masterVolume = 0.7f;
    int sampleRate = 44100;
};

} // namespace AirGuitar
