#include "KarplusStrong.h"
#include <algorithm>
#include <cmath>

namespace AirGuitar {

KarplusStrong::KarplusStrong()
    : rng(42)
{
}

void KarplusStrong::init(float frequency, int sampleRate)
{
    if (frequency <= 0.0f || sampleRate <= 0)
        return;

    delayLength = static_cast<int>(std::round(static_cast<float>(sampleRate) / frequency));
    delayLength = std::max(2, delayLength);
    delayLine.resize(delayLength, 0.0f);
    writePos = 0;
    active = false;
}

void KarplusStrong::pluck(float velocity)
{
    if (delayLine.empty())
        return;

    velocity = std::max(0.0f, std::min(1.0f, velocity));

    for (int i = 0; i < delayLength; ++i)
        delayLine[i] = noise(rng) * velocity;

    writePos = 0;
    active = true;
}

float KarplusStrong::nextSample()
{
    if (!active || delayLine.empty())
        return 0.0f;

    int readPos = writePos;
    int nextPos = (writePos + 1) % delayLength;

    float current = delayLine[readPos];
    float next = delayLine[nextPos];
    float output = (current + next) * 0.5f;

    delayLine[writePos] = output * rho;
    writePos = nextPos;

    if (std::abs(output) < 0.0001f)
        active = false;

    return output;
}

void KarplusStrong::setDecay(float r)
{
    rho = std::max(0.0f, std::min(1.0f, r));
}

void KarplusStrong::reset()
{
    std::fill(delayLine.begin(), delayLine.end(), 0.0f);
    writePos = 0;
    active = false;
}

} // namespace AirGuitar
