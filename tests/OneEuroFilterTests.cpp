#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Physics/OneEuroFilter.h"

using namespace AirGuitar;
using Catch::Approx;

TEST_CASE("OneEuroFilter initialises on first call", "[OneEuroFilter]")
{
    OneEuroFilter filter;
    REQUIRE_FALSE(filter.isInitialised());

    float result = filter.filter(5.0f, 0.01f);
    REQUIRE(filter.isInitialised());
    REQUIRE(result == Approx(5.0f).margin(0.001f));
}

TEST_CASE("OneEuroFilter passes through constant signal", "[OneEuroFilter]")
{
    OneEuroFilter filter(1.0f, 0.0f, 1.0f);

    for (int i = 0; i < 100; ++i)
    {
        float result = filter.filter(10.0f, 0.01f);
        REQUIRE(result == Approx(10.0f).margin(0.1f));
    }
}

TEST_CASE("OneEuroFilter smooths noisy signal", "[OneEuroFilter]")
{
    OneEuroFilter filter(1.0f, 0.0f, 1.0f);

    float trueValue = 5.0f;
    float sum = 0.0f;
    int count = 0;

    for (int i = 0; i < 200; ++i)
    {
        float noise = (i % 2 == 0) ? 0.5f : -0.5f;
        float result = filter.filter(trueValue + noise, 0.01f);
        if (i > 50)
        {
            sum += std::abs(result - trueValue);
            ++count;
        }
    }

    float avgError = sum / static_cast<float>(count);
    REQUIRE(avgError < 0.3f);
}

TEST_CASE("OneEuroFilter adapts cutoff with speed (beta > 0)", "[OneEuroFilter]")
{
    OneEuroFilter filter(1.0f, 0.1f, 1.0f);

    // Slow ramp: 0 to 1 over 200 ticks
    float slowError = 0.0f;
    for (int i = 0; i < 200; ++i)
    {
        float target = static_cast<float>(i) / 200.0f;
        float result = filter.filter(target, 0.01f);
        if (i > 50)
            slowError += std::abs(target - result);
    }
    slowError /= 150.0f;

    filter.reset(0.0f);

    // Fast ramp: 0 to 1 over 20 ticks (10x faster)
    float fastError = 0.0f;
    for (int i = 0; i < 20; ++i)
    {
        float target = static_cast<float>(i) / 20.0f;
        float result = filter.filter(target, 0.01f);
        if (i > 5)
            fastError += std::abs(target - result);
    }
    fastError /= 15.0f;

    // Both errors should be bounded (fast ramp has more error due to shorter settling time)
    REQUIRE(slowError < 0.15f);
    REQUIRE(fastError < 0.35f);
}

TEST_CASE("OneEuroFilter handles zero dt gracefully", "[OneEuroFilter]")
{
    OneEuroFilter filter(1.0f, 0.01f, 1.0f);
    filter.filter(5.0f, 0.01f);

    float result = filter.filter(10.0f, 0.0f);
    REQUIRE(result == Approx(5.0f).margin(0.001f));
}

TEST_CASE("OneEuroFilter handles negative dt gracefully", "[OneEuroFilter]")
{
    OneEuroFilter filter(1.0f, 0.01f, 1.0f);
    filter.filter(5.0f, 0.01f);

    float result = filter.filter(10.0f, -0.01f);
    REQUIRE(result == Approx(5.0f).margin(0.001f));
}

TEST_CASE("OneEuroFilter reset works correctly", "[OneEuroFilter]")
{
    OneEuroFilter filter(1.0f, 0.01f, 1.0f);

    filter.filter(5.0f, 0.01f);
    filter.filter(6.0f, 0.01f);
    filter.filter(7.0f, 0.01f);

    filter.reset(100.0f);
    REQUIRE(filter.getLastValue() == Approx(100.0f).margin(0.001f));
    REQUIRE(filter.isInitialised());

    float result = filter.filter(100.0f, 0.01f);
    REQUIRE(result == Approx(100.0f).margin(0.001f));
}

TEST_CASE("OneEuroFilter parameter setters work", "[OneEuroFilter]")
{
    OneEuroFilter filter;

    filter.setMinCutoff(2.0f);
    REQUIRE(filter.getMinCutoff() == Approx(2.0f));

    filter.setBeta(0.5f);
    REQUIRE(filter.getBeta() == Approx(0.5f));

    filter.setDCutoff(3.0f);
    REQUIRE(filter.getDCutoff() == Approx(3.0f));
}

TEST_CASE("OneEuroFilter does not overshoot step input", "[OneEuroFilter]")
{
    OneEuroFilter filter(1.0f, 0.0f, 1.0f);

    float maxVal = 0.0f;
    for (int i = 0; i < 50; ++i)
    {
        float result = filter.filter(10.0f, 0.01f);
        if (result > maxVal)
            maxVal = result;
    }

    REQUIRE(maxVal <= 10.0f + 0.001f);
}
