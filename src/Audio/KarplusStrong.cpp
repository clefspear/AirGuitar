#include "KarplusStrong.h"
#include <algorithm>
#include <cmath>

namespace AirGuitar {

KarplusStrong::KarplusStrong()
    : rng(s_nextSeed++)
{
}

void KarplusStrong::init(float frequency, int sampleRate)
{
    if (frequency <= 0.0f || sampleRate <= 0)
        return;

    float exactDelay = static_cast<float>(sampleRate) / frequency;
    delayLength = static_cast<int>(std::round(exactDelay));
    delayLength = std::max(2, delayLength);
    fractionalDelay = exactDelay - static_cast<float>(delayLength);
    delayLine.resize(delayLength, 0.0f);
    writePos = 0;
    active = false;

    avgAlpha = 0.5f;
}

void KarplusStrong::pluck(float velocity)
{
    if (delayLine.empty())
        return;

    velocity = std::max(0.01f, std::min(1.0f, velocity));

    if (active && !delayLine.empty())
    {
        prevDelayLine = delayLine;
        crossfadeLength = std::min(delayLength / 4, 256);
        crossfadePos = 0;
        crossfading = crossfadeLength > 0;
    }

    for (int i = 0; i < delayLength; ++i)
    {
        float sample = noise(rng);

        float brightness = 0.3f + 0.7f * velocity;
        float prev = (i > 0) ? delayLine[i - 1] : 0.0f;
        sample = sample * brightness + prev * (1.0f - brightness) * 0.3f;

        delayLine[i] = sample * velocity;
    }

    writePos = 0;
    active = true;
}

float KarplusStrong::nextSample()
{
    if (!active || delayLine.empty())
        return 0.0f;

    int readPos = writePos;
    int nextPos = (writePos + 1) % delayLength;

    float fracAlpha = 0.5f + fractionalDelay * 0.5f;
    float current = delayLine[readPos];
    float next = delayLine[nextPos];
    float output = current * (1.0f - fracAlpha) + next * fracAlpha;

    delayLine[writePos] = output * rho;
    writePos = nextPos;

    if (crossfading && !prevDelayLine.empty())
    {
        int prevLen = static_cast<int>(prevDelayLine.size());
        int prevRead = readPos % prevLen;
        int prevNext = (prevRead + 1) % prevLen;
        float prevOutput = prevDelayLine[prevRead] * (1.0f - fracAlpha)
                         + prevDelayLine[prevNext] * fracAlpha;

        float t = static_cast<float>(crossfadePos) / static_cast<float>(crossfadeLength);
        output = prevOutput * (1.0f - t) + output * t;

        ++crossfadePos;
        if (crossfadePos >= crossfadeLength)
        {
            crossfading = false;
            prevDelayLine.clear();
            prevDelayLine.shrink_to_fit();
        }
    }

    if (std::abs(output) < 0.0001f)
        active = false;

    return output;
}

void KarplusStrong::setDecay(float r)
{
    rho = std::max(0.0f, std::min(1.0f, r));
}

void KarplusStrong::setDamping(float d)
{
    damping = std::max(0.0f, std::min(1.0f, d));
}

void KarplusStrong::reset()
{
    std::fill(delayLine.begin(), delayLine.end(), 0.0f);
    writePos = 0;
    active = false;
    crossfading = false;
    crossfadePos = 0;
    prevDelayLine.clear();
    prevDelayLine.shrink_to_fit();
}

} // namespace AirGuitar
