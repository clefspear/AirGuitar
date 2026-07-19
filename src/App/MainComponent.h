#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_opengl/juce_opengl.h>

#include "Vision/HandPipeline.h"
#include "Vision/Camera.h"
#include "Physics/PhysicsEngine.h"
#include "Physics/NoteEvent.h"
#include "Audio/AudioEngine.h"
#include "Audio/MidiOutput.h"
#include "Calibration/CalibrationManager.h"
#include "UI/CalibrationWizard.h"
#include <atomic>

namespace AirGuitar {

enum class InitError
{
    None,
    CameraNotFound,
    CameraPermissionDenied,
    CameraOpenFailed,
    ModelLoadFailed,
    PipelineInitFailed,
    AudioInitFailed,
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
    bool keyPressed(const juce::KeyPress& key) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

private:
    std::unique_ptr<Camera> camera;
    std::unique_ptr<HandPipeline> handPipeline;
    std::unique_ptr<PhysicsEngine> physicsEngine;
    std::unique_ptr<AudioEngine> audioEngine;
    std::unique_ptr<AirGuitarMidiOutput> midiOutput;
    std::unique_ptr<CalibrationWizard> calWizard;
    CalibrationManager calibrationManager;

    bool cameraReady = false;
    double inferenceFps = 0.0;
    double captureFps = 0.0;
    int frameCount = 0;

    CalibrationData calibration;
    InitError initError = InitError::None;
    std::string initErrorMessage;
    std::string audioInitError;
    std::atomic<bool> midiConnected{false};
    bool funMode = false;
    bool showHelp = false;
    bool mouseInStrumZone = false;
    int mouseNoteString = -1;

    PhysicsState lastState;
    float noteOpacity = 0.0f;
    float glowPhase = 0.0f;
    int cameraRetryCount = 0;
    bool cameraStartFailed = false;

    void drawDebugOverlay(juce::Graphics&);
    void drawErrorOverlay(juce::Graphics&);
    void drawLandmarks(juce::Graphics&, const HandLandmarks&, const juce::Colour&, const PhysicsState&);
    void drawPose(juce::Graphics&, const PoseLandmarks&);
    void drawChordOverlay(juce::Graphics&, const PhysicsState&);
    void drawStrumZone(juce::Graphics&);
    void drawStrumIndicator(juce::Graphics&, const PhysicsState&);
    void drawFretZone(juce::Graphics&, const PhysicsState&);
    void drawNoteDisplay(juce::Graphics&, const PhysicsState&);
    void drawHelpOverlay(juce::Graphics&);
    void drawFunModeIndicator(juce::Graphics&);

    juce::Rectangle<float> getCameraArea() const;

    static juce::File findModelsDirectory();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace AirGuitar
