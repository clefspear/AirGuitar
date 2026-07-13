#include "OneEuroFilter.h"

namespace AirGuitar {

OneEuroFilter::OneEuroFilter(float minCutoff, float beta, float dCutoff)
    : minCutoff(minCutoff)
    , beta(beta)
    , dCutoff(dCutoff)
{
}

void OneEuroFilter::reset(float initialValue)
{
    xPrev = initialValue;
    dxPrev = 0.0f;
    tPrev = -1.0f;
    initialised = true;
}

float OneEuroFilter::filter(float value, float dt)
{
    if (!initialised)
    {
        reset(value);
        return value;
    }

    if (dt <= 0.0f)
        return xPrev;

    float alphaD = computeAlpha(dCutoff, dt);
    float dx = (value - xPrev) / dt;
    float dxHat = alphaD * dx + (1.0f - alphaD) * dxPrev;

    float cutoff = minCutoff + beta * std::abs(dxHat);
    float alpha = computeAlpha(cutoff, dt);

    float xHat = alpha * value + (1.0f - alpha) * xPrev;

    xPrev = xHat;
    dxPrev = dxHat;

    return xHat;
}

void OneEuroFilter::setMinCutoff(float cutoff)
{
    minCutoff = cutoff;
}

void OneEuroFilter::setBeta(float b)
{
    beta = b;
}

void OneEuroFilter::setDCutoff(float cutoff)
{
    dCutoff = cutoff;
}

float OneEuroFilter::computeAlpha(float cutoff, float dt)
{
    float tau = 1.0f / (2.0f * 3.14159265f * cutoff);
    return 1.0f / (1.0f + tau / dt);
}

} // namespace AirGuitar
