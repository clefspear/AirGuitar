#include "BodyResonance.h"
#include <cmath>

namespace AirGuitar {

BodyResonance::BodyResonance() {}

void BodyResonance::init(int sampleRate)
{
    float resonanceHz = 95.0f;
    int delaySamples = static_cast<int>(std::round(
        static_cast<float>(sampleRate) / resonanceHz));
    delaySamples = std::max(2, delaySamples);

    delayLine.resize(delaySamples, 0.0f);
    writePos = 0;
    filterState = 0.0f;
}

float BodyResonance::process(float input)
{
    if (delayLine.empty())
        return input;

    int readPos = writePos;
    float delayed = delayLine[readPos];

    float cutoff = 0.3f;
    filterState += cutoff * (delayed - filterState);
    float filtered = filterState;

    delayLine[writePos] = input + filtered * feedback;
    writePos = (writePos + 1) % static_cast<int>(delayLine.size());

    return input + filtered * dampening;
}

void BodyResonance::reset()
{
    std::fill(delayLine.begin(), delayLine.end(), 0.0f);
    writePos = 0;
    filterState = 0.0f;
}

} // namespace AirGuitar
