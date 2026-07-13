#pragma once
#include <cmath>
#include <algorithm>

namespace AirGuitar {

class OneEuroFilter
{
public:
    OneEuroFilter() = default;

    OneEuroFilter(float minCutoff, float beta, float dCutoff);

    void reset(float initialValue);

    float filter(float value, float dt);

    void setMinCutoff(float cutoff);
    void setBeta(float beta);
    void setDCutoff(float cutoff);

    float getMinCutoff() const { return minCutoff; }
    float getBeta() const { return beta; }
    float getDCutoff() const { return dCutoff; }
    float getLastValue() const { return xPrev; }
    bool isInitialised() const { return initialised; }

private:
    float minCutoff = 1.0f;
    float beta = 0.01f;
    float dCutoff = 1.0f;

    float xPrev = 0.0f;
    float dxPrev = 0.0f;
    float tPrev = -1.0f;

    bool initialised = false;

    static float computeAlpha(float cutoff, float dt);
};

} // namespace AirGuitar
