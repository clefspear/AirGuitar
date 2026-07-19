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
        Handedness = 1,
        Fret1 = 2,
        Fret12 = 3,
        StrumZone = 4,
        Complete = 5
    };

    Step step = Step::None;
    bool completed = false;
    CalibrationData result;

    FrameData latestLandmarks;

    float fret1X = 0.0f;
    float fret12X = 0.0f;
    float fretMinY = 1.0f;
    float fretMaxY = 0.0f;
    float strumLeft = 0.0f;
    float strumRight = 0.0f;
    float strumTop = 0.0f;
    float strumBottom = 0.0f;

    int dwellFrames = 0;
    int graceFrames = 0;
    int welcomeFrames = 0;
    bool handednessAutoDetected = false;
    static constexpr int kDwellRequired = 30;
    static constexpr int kGraceFrames = 5;
    static constexpr int kWelcomeAutoAdvanceFrames = 60;

    bool fretCaptured = false;
    bool strumCaptured = false;

    CompleteCallback onComplete;

    void advanceStep();
    void computeCalibration();
    const HandLandmarks* findFretHand() const;
    const HandLandmarks* findStrumHand() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CalibrationWizard)
};

} // namespace AirGuitar
