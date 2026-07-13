#pragma once
#include <vector>
#include <cstdint>
#include <random>

namespace AirGuitar {

class KarplusStrong
{
public:
    KarplusStrong();

    void init(float frequency, int sampleRate);
    void pluck(float velocity);
    float nextSample();
    void setDecay(float rho);
    bool isActive() const { return active; }

    void reset();

private:
    std::vector<float> delayLine;
    int writePos = 0;
    int delayLength = 0;
    float rho = 0.996f;
    bool active = false;

    std::mt19937 rng;
    std::uniform_real_distribution<float> noise{-1.0f, 1.0f};
};

} // namespace AirGuitar
