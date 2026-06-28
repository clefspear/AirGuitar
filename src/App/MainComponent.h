#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_opengl/juce_opengl.h>

#include "Vision/HandPipeline.h"
#include "Vision/Camera.h"

namespace AirGuitar {

class MainComponent : public juce::Component,
                      public juce::Timer,
                      public juce::MessageListener
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void handleMessage(const juce::Message&) override;

private:
    std::unique_ptr<Camera> camera;
    std::unique_ptr<HandPipeline> handPipeline;
    bool cameraReady = false;
    double inferenceFps = 0.0;
    double captureFps = 0.0;
    int frameCount = 0;

    void drawDebugOverlay(juce::Graphics&);
    void drawLandmarks(juce::Graphics&, const HandLandmarks&, const juce::Colour&);
    void drawPose(juce::Graphics&, const PoseLandmarks&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace AirGuitar
