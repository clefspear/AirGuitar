#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include "Physics/CalibrationData.h"
#include "Vision/LandmarkData.h"
#include <functional>

namespace AirGuitar {

class CalibrationWizard : public juce::Component,
                          public juce::Timer
{
public:
    CalibrationWizard();
    ~CalibrationWizard() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    bool keyPressed(const juce::KeyPress& key) override;

    void start();
    void cancel();
    bool isActive() const { return step != Step::None; }
    bool isComplete() const { return completed; }

    void feedLandmarks(const FrameData& landmarks);

    using CompleteCallback = std::function<void(const CalibrationData&)>;
    void setCompleteCallback(CompleteCallback cb);

    CalibrationData getResult() const { return result; }

private:
    enum class Step
    {
        None = -1,
        Welcome = 0,
        Fret1 = 1,
        Fret12 = 2,
        StrumZone = 3,
        Complete = 4
    };

    Step step = Step::None;
    bool completed = false;
    CalibrationData result;

    FrameData latestLandmarks;

    float fret1X = 0.0f;
    float fret12X = 0.0f;
    float strumLeft = 0.0f;
    float strumRight = 0.0f;
    float strumTop = 0.0f;
    float strumBottom = 0.0f;

    int dwellFrames = 0;
    static constexpr int kDwellRequired = 30;

    CompleteCallback onComplete;

    void advanceStep();
    void computeCalibration();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CalibrationWizard)
};

} // namespace AirGuitar
