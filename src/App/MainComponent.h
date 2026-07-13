#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_opengl/juce_opengl.h>

#include "Vision/HandPipeline.h"
#include "Vision/Camera.h"
#include "Physics/PhysicsEngine.h"
#include "Audio/AudioEngine.h"
#include "Calibration/CalibrationManager.h"

namespace AirGuitar {

enum class InitError
{
    None,
    CameraNotFound,
    CameraPermissionDenied,
    CameraOpenFailed,
    ModelLoadFailed,
    PipelineInitFailed,
};

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
    std::unique_ptr<PhysicsEngine> physicsEngine;
    std::unique_ptr<AudioEngine> audioEngine;
    CalibrationManager calibrationManager;

    bool cameraReady = false;
    double inferenceFps = 0.0;
    double captureFps = 0.0;
    int frameCount = 0;

    CalibrationData calibration;
    InitError initError = InitError::None;
    std::string initErrorMessage;

    void drawDebugOverlay(juce::Graphics&);
    void drawErrorOverlay(juce::Graphics&);
    void drawLandmarks(juce::Graphics&, const HandLandmarks&, const juce::Colour&);
    void drawPose(juce::Graphics&, const PoseLandmarks&);
    void drawChordOverlay(juce::Graphics&, const PhysicsState&);
    void drawStrumZone(juce::Graphics&);

    static juce::File findModelsDirectory();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace AirGuitar
