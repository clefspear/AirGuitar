#pragma once
#include <vector>

namespace AirGuitar {

class BodyResonance
{
public:
    BodyResonance();

    void init(int sampleRate);
    float process(float input);
    void reset();

private:
    std::vector<float> delayLine;
    int writePos = 0;
    float feedback = 0.3f;
    float dampening = 0.5f;
    float filterState = 0.0f;
};

} // namespace AirGuitar
