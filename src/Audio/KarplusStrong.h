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
    void setDamping(float damping);
    bool isActive() const { return active; }

    void reset();

private:
    std::vector<float> delayLine;
    std::vector<float> prevDelayLine;
    int writePos = 0;
    int delayLength = 0;
    float fractionalDelay = 0.0f;
    float rho = 0.996f;
    bool active = false;

    float avgAlpha = 0.5f;
    float damping = 0.5f;

    int crossfadeLength = 0;
    int crossfadePos = 0;
    bool crossfading = false;

    std::mt19937 rng;
    std::uniform_real_distribution<float> noise{-1.0f, 1.0f};

    static inline int s_nextSeed = 42;
};

} // namespace AirGuitar
