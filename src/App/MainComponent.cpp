#include "MainComponent.h"
#include "Application.h"
#include "CrashLogger.h"
#include <opencv2/imgcodecs.hpp>
#include <chrono>
#include <cmath>

namespace AirGuitar {

MainComponent::MainComponent()
{
    setSize(1280, 720);

    auto appDir = juce::File::getSpecialLocation(
        juce::File::currentExecutableFile).getParentDirectory()
        .getFullPathName().toStdString();
    CrashLogger::instance().init(appDir);
    CrashLogger::instance().installSignalHandlers();
    CrashLogger::instance().logInfo("Application starting");

    calibration = CalibrationData::defaultConfig();
    calibrationManager.load(calibration, "");

    physicsEngine = std::make_unique<PhysicsEngine>();
    physicsEngine->setCalibration(calibration);

    audioEngine = std::make_unique<AudioEngine>();
    midiOutput = std::make_unique<AirGuitarMidiOutput>();

    calWizard = std::make_unique<CalibrationWizard>();
    calWizard->setCompleteCallback([this](const CalibrationData& cal) {
        calibration = cal;
        physicsEngine->setCalibration(cal);
        calibrationManager.save(cal, "");
        CrashLogger::instance().logInfo("Calibration saved");
    });

    auto devices = AirGuitarMidiOutput::getAvailableDevices();
    if (!devices.empty())
    {
        midiConnected = midiOutput->openDevice(devices[0]);
        if (midiConnected)
            CrashLogger::instance().logInfo("MIDI connected: " + devices[0]);
    }

    physicsEngine->setNoteCallback([this](const NoteEvent& evt) {
        if (audioEngine)
            audioEngine->handleNoteEvent(evt);
        if (midiOutput && midiConnected)
            midiOutput->processNoteEvent(evt);
    });

    physicsEngine->start();
    CrashLogger::instance().logInfo("Physics engine started");

    camera = std::make_unique<Camera>();

    auto result = camera->open(0, Camera::kDefaultWidth,
                                Camera::kDefaultHeight,
                                Camera::kDefaultFps);

    if (result == Camera::Error::None)
    {
        CrashLogger::instance().logInfo("Camera opened successfully");

        auto modelsDir = findModelsDirectory();
        CrashLogger::instance().logInfo("Models directory: " + modelsDir.getFullPathName().toStdString());

        if (!modelsDir.isDirectory())
        {
            initError = InitError::ModelLoadFailed;
            initErrorMessage = "Models directory not found at: " + modelsDir.getFullPathName().toStdString();
            CrashLogger::instance().logError("Init", initErrorMessage);
        }
        else
        {
            handPipeline = std::make_unique<HandPipeline>();
            auto loadResult = handPipeline->initialise(
                modelsDir.getFullPathName().toStdString());

            if (loadResult == HandPipeline::Error::None)
            {
                cameraReady = true;
                handPipeline->start();

                handPipeline->setFrameCallback(
                    [this](const FrameData& frame)
                    {
                        if (physicsEngine)
                            physicsEngine->pushFrame(frame);
                    });

                camera->setFrameCallback(
                    [this](const cv::Mat& frame, int64_t timestampMs)
                    {
                        if (handPipeline && handPipeline->isRunning())
                            handPipeline->onFrameReceived(frame, timestampMs);
                    });

                camera->startCapture();
                CrashLogger::instance().logInfo("Hand pipeline started, camera capture running");

                if (!calibrationManager.hasSavedCalibration())
                {
                    calWizard->start();
                    CrashLogger::instance().logInfo("Auto-starting calibration wizard (no saved calibration)");
                }
            }
            else
            {
                initError = InitError::PipelineInitFailed;
                initErrorMessage = "Hand pipeline init failed (error " +
                    std::to_string(static_cast<int>(loadResult)) + ")";
                CrashLogger::instance().logError("Init", initErrorMessage);
            }
        }
    }
    else
    {
        switch (result)
        {
            case Camera::Error::NotFound:
                initError = InitError::CameraNotFound;
                initErrorMessage = "No camera found. Please connect a webcam.";
                break;
            case Camera::Error::PermissionDenied:
                initError = InitError::CameraPermissionDenied;
                initErrorMessage = "Camera permission denied. Grant access in System Settings > Privacy & Security > Camera.";
                cameraStartFailed = true;
                break;
            default:
                initError = InitError::CameraOpenFailed;
                initErrorMessage = "Camera failed to open (error " +
                    std::to_string(static_cast<int>(result)) + ")";
                cameraStartFailed = true;
                break;
        }
        CrashLogger::instance().logError("Init", initErrorMessage);
    }

    if (audioEngine)
    {
        audioEngine->startAudio();
        if (audioEngine->isInitialised())
        {
            CrashLogger::instance().logInfo("Audio engine started");
        }
        else
        {
            audioInitError = audioEngine->getStartError();
            initError = InitError::AudioInitFailed;
            initErrorMessage = "Audio device failed: " + audioInitError;
            CrashLogger::instance().logError("Init", initErrorMessage);
        }
    }

    {
        std::ostringstream ss;
        ss << "Calibration loaded:"
           << " leftHandFretting=" << calibration.leftHandFretting
           << " strumZone=[" << calibration.strumZoneLeft << "," << calibration.strumZoneRight << "]"
           << "x[" << calibration.strumZoneTop << "," << calibration.strumZoneBottom << "]"
           << " strings=[" << calibration.stringTopY << "," << calibration.stringBottomY << "]"
           << " fret1X=" << calibration.fret1X
           << " fret12X=" << calibration.fret12X;
        CrashLogger::instance().logInfo(ss.str());
    }

    startTimerHz(30);
    setWantsKeyboardFocus(true);
    grabKeyboardFocus();
}

MainComponent::~MainComponent()
{
    stopTimer();
    CrashLogger::instance().logInfo("Shutting down");
    if (physicsEngine) physicsEngine->stop();
    if (midiOutput) midiOutput->closeDevice();
    if (handPipeline) handPipeline->stop();
    if (camera) camera->stopCapture();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    if (cameraReady && camera && handPipeline)
    {
        auto latestFrame = camera->getLatestFrame();
        if (!latestFrame.image.empty())
        {
            const auto& frame = latestFrame.image;

#ifdef AIRGUITAR_DEBUG_FRAME_DUMP
            static std::atomic<bool> debugDumpEnabled{true};
            static int debugFrameCount = 0;
            if (debugDumpEnabled.load(std::memory_order_relaxed) && debugFrameCount < 5)
            {
                auto dumpPath = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                    .getParentDirectory().getChildFile("debug_frame_" + juce::String(debugFrameCount) + ".png");
                cv::imwrite(dumpPath.getFullPathName().toStdString(), frame);
                std::cout << "Debug: saved frame " << debugFrameCount << " to " << dumpPath.getFullPathName().toStdString() << std::endl;
                ++debugFrameCount;
            }
#endif

            auto img = juce::Image(juce::Image::PixelFormat::ARGB,
                                   frame.cols, frame.rows, false);

            juce::Image::BitmapData bitmapData(img,
                juce::Image::BitmapData::writeOnly);

            for (int y = 0; y < frame.rows; ++y)
            {
                const auto* src = frame.ptr<unsigned char>(y);
                auto* dst = reinterpret_cast<juce::PixelARGB*>(
                    bitmapData.getLinePointer(y));
                for (int x = 0; x < frame.cols; ++x)
                {
                    dst[x].setARGB(255,
                                   src[x * 3 + 0],
                                   src[x * 3 + 1],
                                   src[x * 3 + 2]);
                }
            }

            auto bounds = getLocalBounds().toFloat();
            auto scaleX = bounds.getWidth() / static_cast<float>(frame.cols);
            auto scaleY = bounds.getHeight() / static_cast<float>(frame.rows);
            auto scale = std::min(scaleX, scaleY);

            auto w = frame.cols * scale;
            auto h = frame.rows * scale;
            auto x = (bounds.getWidth() - w) / 2.0f;
            auto yPos = (bounds.getHeight() - h) / 2.0f;

            g.drawImage(img, x, yPos, w, h, 0, 0,
                        frame.cols, frame.rows);

            auto landmarks = handPipeline->getLatestLandmarks();

            if (calWizard && calWizard->isActive())
                calWizard->feedLandmarks(landmarks);

            for (const auto& hand : landmarks.hands)
            {
                juce::Colour colour = hand.isLeft
                    ? juce::Colours::cyan
                    : juce::Colours::orange;
                drawLandmarks(g, hand, colour, lastState);
            }
            drawPose(g, landmarks.pose);
        }
    }

    if (physicsEngine && cameraReady)
    {
        drawStrumZone(g);
        drawStrumIndicator(g, lastState);
        drawFretZone(g, lastState);
        drawNoteDisplay(g, lastState);
        drawChordOverlay(g, lastState);
        drawFunModeIndicator(g);
    }

    if (showHelp)
        drawHelpOverlay(g);

    if (calWizard && calWizard->isActive())
    {
        calWizard->setBounds(getLocalBounds());
        calWizard->paint(g);
    }

    if (initError != InitError::None)
        drawErrorOverlay(g);

    drawDebugOverlay(g);
}

void MainComponent::resized()
{
}

void MainComponent::timerCallback()
{
    ++frameCount;

    if (cameraStartFailed && camera)
    {
        cameraRetryCount++;
        if (cameraRetryCount % 30 == 0)
        {
            CrashLogger::instance().logInfo("Retrying camera open...");
            auto err = camera->reopen();
            if (err == Camera::Error::None)
            {
                cameraReady = true;
                cameraStartFailed = false;
                CrashLogger::instance().logInfo("Camera opened on retry");
            }
        }
    }

    if (frameCount % 30 == 0)
    {
        if (handPipeline) inferenceFps = handPipeline->getInferenceRate();
        if (camera) captureFps = camera->getFrameRate();
    }

    if (physicsEngine)
        lastState = physicsEngine->getLatestState();

    if (lastState.tracking && !lastState.chordName.empty())
        noteOpacity = std::min(1.0f, noteOpacity + 0.08f);
    else
        noteOpacity = std::max(0.0f, noteOpacity - 0.12f);

    glowPhase += 0.05f;
    if (glowPhase > 6.28318f)
        glowPhase -= 6.28318f;

    repaint();
}

void MainComponent::handleMessage(const juce::Message&)
{
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        if (calWizard && calWizard->isActive())
        {
            calWizard->keyPressed(key);
            return true;
        }
        if (showHelp)
        {
            showHelp = false;
            return true;
        }
        return false;
    }

    if (key == juce::KeyPress('?') || key == juce::KeyPress('/'))
    {
        showHelp = !showHelp;
        return true;
    }

    if (key == juce::KeyPress('f') || key == juce::KeyPress('F'))
    {
        funMode = !funMode;
        calibration.funMode = funMode;
        physicsEngine->setCalibration(calibration);
        CrashLogger::instance().logInfo("Fun mode: " + std::string(funMode ? "ON" : "OFF"));
        return true;
    }

    if (key == juce::KeyPress('c') || key == juce::KeyPress('C'))
    {
        if (calWizard && !calWizard->isActive())
        {
            calWizard->start();
            calWizard->repaint();
            return true;
        }
    }

    if (key == juce::KeyPress('m') || key == juce::KeyPress('M'))
    {
        if (midiConnected)
        {
            midiOutput->closeDevice();
            midiConnected = false;
        }
        else
        {
            auto devices = AirGuitarMidiOutput::getAvailableDevices();
            if (!devices.empty())
                midiConnected = midiOutput->openDevice(devices[0]);
        }
        return true;
    }

    if (calWizard && calWizard->isActive())
    {
        calWizard->keyPressed(key);
        return true;
    }

    return false;
}

juce::File MainComponent::findModelsDirectory()
{
    auto exeDir = juce::File::getSpecialLocation(
        juce::File::currentExecutableFile).getParentDirectory();

    // Strategy 1: Inside .app bundle, check Resources/ (bundled models)
    auto bundleResources = exeDir.getChildFile("../../Resources/models");
    if (bundleResources.isDirectory())
        return bundleResources;

    // Strategy 2: Walk up to find project root models/
    auto searchDir = exeDir;
    for (int i = 0; i < 8; ++i)
    {
        auto candidate = searchDir.getChildFile("models");
        if (candidate.isDirectory())
            return candidate;
        searchDir = searchDir.getParentDirectory();
        if (!searchDir.isDirectory())
            break;
    }

    // Strategy 3: Fallback to source tree relative path
    return exeDir.getChildFile("../../../../../../models");
}

void MainComponent::drawDebugOverlay(juce::Graphics& g)
{
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::Font(juce::FontOptions(14.0f)));

    auto x = 10;
    auto y = 10;
    auto lineH = 18;

    g.drawText("AirGuitar v0.1.0", x, y, 200, lineH,
               juce::Justification::left);
    y += lineH;

    if (cameraReady)
    {
        g.drawText("Capture: " + juce::String(captureFps, 1) + " fps",
                   x, y, 200, lineH, juce::Justification::left);
        y += lineH;
        g.drawText("Inference: " + juce::String(inferenceFps, 1) + " fps",
                   x, y, 200, lineH, juce::Justification::left);
        y += lineH;

        auto landmarks = handPipeline
            ? handPipeline->getLatestLandmarks()
            : FrameData{};
        g.drawText("Hands: " + juce::String(landmarks.hands.size()),
                   x, y, 200, lineH, juce::Justification::left);
        y += lineH;

        if (!landmarks.hands.empty())
        {
            auto& first = landmarks.hands[0];
            g.drawText("Left hand: " + juce::String(first.isLeft ? "yes" : "no"),
                       x, y, 200, lineH, juce::Justification::left);
            y += lineH;
        }
    }

    g.setColour(midiConnected ? juce::Colours::limegreen : juce::Colours::grey);
    g.drawText("MIDI: " + juce::String(midiConnected ? "connected" : "none"),
               x, y, 200, lineH, juce::Justification::left);
}

void MainComponent::drawErrorOverlay(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.fillRect(bounds);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(24.0f)));

    std::string title;
    switch (initError)
    {
        case InitError::CameraNotFound:     title = "No Camera Found"; break;
        case InitError::CameraPermissionDenied: title = "Camera Permission Required"; break;
        case InitError::CameraOpenFailed:   title = "Camera Error"; break;
        case InitError::ModelLoadFailed:    title = "Models Not Found"; break;
        case InitError::PipelineInitFailed: title = "Pipeline Init Failed"; break;
        case InitError::AudioInitFailed:    title = "Audio Device Error"; break;
        default: break;
    }

    auto textBounds = bounds.reduced(40);
    g.drawText(title, textBounds.removeFromTop(40), juce::Justification::centred);

    g.setFont(juce::Font(juce::FontOptions(16.0f)));
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawText(initErrorMessage, textBounds, juce::Justification::centred);

    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.setColour(juce::Colours::yellow.withAlpha(0.6f));
    g.drawText("Crash log: " + CrashLogger::instance().getLogPath(),
               textBounds, juce::Justification::centredBottom);

    if (cameraStartFailed)
    {
        g.setFont(juce::Font(juce::FontOptions(14.0f)));
        g.setColour(juce::Colours::green.withAlpha(0.8f));
        g.drawText("Retrying camera in background...",
                   textBounds, juce::Justification::centredBottom);
    }
}

void MainComponent::drawLandmarks(juce::Graphics& g,
                                   const HandLandmarks& hand,
                                   const juce::Colour& colour,
                                   const PhysicsState& state)
{
    if (hand.landmarks.size() != 21)
        return;

    auto bounds = getLocalBounds().toFloat();

    int camW = camera ? camera->getWidth() : Camera::kDefaultWidth;
    int camH = camera ? camera->getHeight() : Camera::kDefaultHeight;
    auto scaleX = bounds.getWidth() / static_cast<float>(camW);
    auto scaleY = bounds.getHeight() / static_cast<float>(camH);
    auto scale = std::min(scaleX, scaleY);
    auto w = camW * scale;
    auto h = camH * scale;
    auto ox = (bounds.getWidth() - w) / 2.0f;
    auto oy = (bounds.getHeight() - h) / 2.0f;

    static constexpr int kFingertipIndices[] = {4, 8, 12, 16, 20};
    static constexpr int kFingerMcpIndices[] = {2, 5, 9, 13, 17};
    static constexpr float kStringYPositions[6] = {};

    for (size_t i = 0; i < hand.landmarks.size(); ++i)
    {
        auto& lm = hand.landmarks[i];
        auto px = ox + lm.x * w;
        auto py = oy + lm.y * h;

        bool isFingertip = false;
        for (int fi = 0; fi < 5; ++fi)
        {
            if (static_cast<int>(i) == kFingertipIndices[fi])
            {
                isFingertip = true;
                break;
            }
        }

        if (isFingertip)
        {
            bool noteActive = false;
            float tipY = lm.y;
            float closestDist = 999.0f;
            for (int s = 0; s < 6; ++s)
            {
                if (state.fretState.activeStrings[s] >= 0)
                {
                    float stringY = calibration.stringTopY
                        + static_cast<float>(s) / 5.0f
                        * (calibration.stringBottomY - calibration.stringTopY);
                    float dist = std::abs(tipY - stringY);
                    if (dist < closestDist)
                        closestDist = dist;
                    if (dist < 0.08f)
                        noteActive = true;
                }
            }

            juce::Colour tipColour = noteActive
                ? juce::Colours::limegreen
                : juce::Colours::red;

            float glow = noteActive ? (0.7f + 0.3f * std::sin(glowPhase * 2.0f)) : 1.0f;
            g.setColour(tipColour.withAlpha(glow));
            g.fillEllipse(px - 9.0f, py - 9.0f, 18.0f, 18.0f);

            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.drawEllipse(px - 9.0f, py - 9.0f, 18.0f, 18.0f, 2.0f);
        }
        else
        {
            g.setColour(colour.withAlpha(0.85f));
            g.fillEllipse(px - 3.5f, py - 3.5f, 7.0f, 7.0f);
        }
    }

    static const std::vector<std::pair<int, int>> connections = {
        {0, 1}, {1, 2}, {2, 3}, {3, 4},
        {0, 5}, {5, 6}, {6, 7}, {7, 8},
        {0, 9}, {9, 10}, {10, 11}, {11, 12},
        {0, 13}, {13, 14}, {14, 15}, {15, 16},
        {0, 17}, {17, 18}, {18, 19}, {19, 20}
    };

    g.setColour(colour.withAlpha(0.5f));
    for (auto [a, b] : connections)
    {
        auto& p1 = hand.landmarks[a];
        auto& p2 = hand.landmarks[b];
        g.drawLine(ox + p1.x * w, oy + p1.y * h,
                   ox + p2.x * w, oy + p2.y * h,
                   2.0f);
    }
}

void MainComponent::drawPose(juce::Graphics& g, const PoseLandmarks& pose)
{
    if (pose.landmarks.size() < 25)
        return;

    auto bounds = getLocalBounds().toFloat();

    int camW = camera ? camera->getWidth() : Camera::kDefaultWidth;
    int camH = camera ? camera->getHeight() : Camera::kDefaultHeight;
    auto scaleX = bounds.getWidth() / static_cast<float>(camW);
    auto scaleY = bounds.getHeight() / static_cast<float>(camH);
    auto scale = std::min(scaleX, scaleY);
    auto w = camW * scale;
    auto h = camH * scale;
    auto ox = (bounds.getWidth() - w) / 2.0f;
    auto oy = (bounds.getHeight() - h) / 2.0f;

    static const std::vector<std::pair<int, int>> connections = {
        {11, 12}, {11, 23}, {12, 24}, {23, 24},
        {11, 13}, {13, 15}, {12, 14}, {14, 16},
        {15, 17}, {15, 19}, {15, 21}, {16, 18},
        {16, 20}, {16, 22}
    };

    g.setColour(juce::Colours::limegreen.withAlpha(0.6f));
    for (auto [a, b] : connections)
    {
        if (static_cast<size_t>(a) >= pose.landmarks.size() ||
            static_cast<size_t>(b) >= pose.landmarks.size())
            continue;

        auto& p1 = pose.landmarks[a];
        auto& p2 = pose.landmarks[b];
        if (p1.visibility < 0.5f || p2.visibility < 0.5f)
            continue;

        g.drawLine(ox + p1.x * w, oy + p1.y * h,
                   ox + p2.x * w, oy + p2.y * h,
                   2.0f);
    }
}

void MainComponent::drawChordOverlay(juce::Graphics& g, const PhysicsState& state)
{
    if (!state.tracking)
        return;

    auto bounds = getLocalBounds().toFloat();

    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::Font(juce::FontOptions(32.0f)));
    g.drawText(state.chordName, bounds.removeFromBottom(50),
               juce::Justification::centred);

    g.setFont(juce::Font(juce::FontOptions(16.0f)));
    g.drawText("Fret: " + juce::String(state.fretState.fret),
               bounds.removeFromBottom(30),
               juce::Justification::centred);
}

juce::Rectangle<float> MainComponent::getCameraArea() const
{
    auto bounds = getLocalBounds().toFloat();
    int camW = camera ? camera->getWidth() : Camera::kDefaultWidth;
    int camH = camera ? camera->getHeight() : Camera::kDefaultHeight;
    auto scaleX = bounds.getWidth() / static_cast<float>(camW);
    auto scaleY = bounds.getHeight() / static_cast<float>(camH);
    auto scale = std::min(scaleX, scaleY);
    auto w = camW * scale;
    auto h = camH * scale;
    auto x = (bounds.getWidth() - w) / 2.0f;
    auto y = (bounds.getHeight() - h) / 2.0f;
    return {x, y, w, h};
}

void MainComponent::drawStrumZone(juce::Graphics& g)
{
    auto cam = getCameraArea();

    float left   = cam.getX() + cam.getWidth()  * calibration.strumZoneLeft;
    float right  = cam.getX() + cam.getWidth()  * calibration.strumZoneRight;
    float top    = cam.getY() + cam.getHeight() * calibration.strumZoneTop;
    float bottom = cam.getY() + cam.getHeight() * calibration.strumZoneBottom;

    g.setColour(juce::Colours::yellow.withAlpha(0.25f));
    g.fillRect(left, top, right - left, bottom - top);

    g.setColour(juce::Colours::yellow.withAlpha(0.7f));
    g.drawRect(left, top, right - left, bottom - top, 2.0f);

    g.setColour(juce::Colours::yellow.withAlpha(0.6f));
    g.setFont(juce::Font(juce::FontOptions(13.0f)));

    for (int i = 0; i < 6; ++i)
    {
        float y = top + static_cast<float>(i) / 5.0f * (bottom - top);
        g.drawLine(left, y, right, y, 1.0f);
        g.drawText("S" + juce::String(i + 1), right + 5, y - 7, 25, 14,
                   juce::Justification::left);
    }
}

void MainComponent::drawStrumIndicator(juce::Graphics& g, const PhysicsState& state)
{
    if (!state.tracking)
        return;

    auto cam = getCameraArea();

    float left   = cam.getX() + cam.getWidth()  * calibration.strumZoneLeft;
    float right  = cam.getX() + cam.getWidth()  * calibration.strumZoneRight;
    float top    = cam.getY() + cam.getHeight() * calibration.strumZoneTop;
    float bottom = cam.getY() + cam.getHeight() * calibration.strumZoneBottom;

    auto landmarks = handPipeline ? handPipeline->getLatestLandmarks() : FrameData{};
    for (const auto& hand : landmarks.hands)
    {
        if (hand.landmarks.empty())
            continue;

        float wx = cam.getX() + hand.landmarks[0].x * cam.getWidth();
        float wy = cam.getY() + hand.landmarks[0].y * cam.getHeight();

        bool inZone = hand.landmarks[0].x >= calibration.strumZoneLeft
                   && hand.landmarks[0].x <= calibration.strumZoneRight
                   && hand.landmarks[0].y >= calibration.strumZoneTop
                   && hand.landmarks[0].y <= calibration.strumZoneBottom;

        if (!inZone)
            continue;

        juce::Colour dotColour = state.strumming
            ? juce::Colours::limegreen
            : juce::Colours::yellow;

        float pulse = 0.7f + 0.3f * std::sin(glowPhase * 3.0f);
        g.setColour(dotColour.withAlpha(pulse));
        g.fillEllipse(wx - 8.0f, wy - 8.0f, 16.0f, 16.0f);

        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.drawEllipse(wx - 8.0f, wy - 8.0f, 16.0f, 16.0f, 2.0f);
    }

    if (state.strumming)
    {
        float cx = (left + right) / 2.0f;
        float cy = top - 15.0f;
        g.setColour(juce::Colours::limegreen.withAlpha(0.8f));
        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        juce::String dir = state.strumDirection == StrumDirection::Down ? "DN" : "UP";
        g.drawText(dir, cx - 10, cy, 20, 12, juce::Justification::centred);
    }
}

void MainComponent::drawFretZone(juce::Graphics& g, const PhysicsState& state)
{
    auto cam = getCameraArea();

    float left   = cam.getX() + cam.getWidth()  * calibration.fret1X;
    float right  = cam.getX() + cam.getWidth()  * calibration.fret12X;
    float top    = cam.getY() + cam.getHeight() * calibration.stringTopY;
    float bottom = cam.getY() + cam.getHeight() * calibration.stringBottomY;

    g.setColour(juce::Colours::green.withAlpha(0.20f));
    g.fillRect(left, top, right - left, bottom - top);

    g.setColour(juce::Colours::green.withAlpha(0.6f));
    g.drawRect(left, top, right - left, bottom - top, 2.0f);

    g.setColour(juce::Colours::green.withAlpha(0.5f));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));

    for (int i = 0; i < 6; ++i)
    {
        float y = top + static_cast<float>(i) / 5.0f * (bottom - top);
        g.drawLine(left, y, right, y, 1.0f);
        g.drawText("S" + juce::String(i + 1), left - 30, y - 7, 27, 14,
                   juce::Justification::right);
    }

    static const int fretMarks[] = {1, 3, 5, 7, 9, 12};
    static const int numMarks = 6;
    for (int i = 0; i < numMarks; ++i)
    {
        int fretNum = fretMarks[i];
        float fretX = left + static_cast<float>(fretNum - 1) / 11.0f * (right - left);
        g.drawLine(fretX, top, fretX, bottom, 1.0f);

        bool isCurrentFret = (state.tracking && state.fretState.fret == fretNum);
        g.setColour(isCurrentFret
            ? juce::Colours::limegreen.withAlpha(0.9f)
            : juce::Colours::green.withAlpha(0.5f));
        g.drawText(juce::String(fretNum), fretX - 10, top - 16, 20, 14,
                   juce::Justification::centred);
    }

    float nutX = left;
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawLine(nutX, top, nutX, bottom, 3.0f);

    if (state.tracking)
    {
        g.setColour(juce::Colours::green.withAlpha(0.7f));
        g.setFont(juce::Font(juce::FontOptions(14.0f)));
        g.drawText("Fret " + juce::String(state.fretState.fret),
                   left, bottom + 5, right - left, 18,
                   juce::Justification::centred);
    }
}

void MainComponent::mouseDown(const juce::MouseEvent& event)
{
    if (showHelp)
    {
        showHelp = false;
        return;
    }

    auto pos = event.getPosition().toFloat();
    auto cam = getCameraArea();
    float szLeft = cam.getX() + cam.getWidth() * calibration.strumZoneLeft;
    float szRight = cam.getX() + cam.getWidth() * calibration.strumZoneRight;
    float szTop = cam.getY() + cam.getHeight() * calibration.strumZoneTop;
    float szBottom = cam.getY() + cam.getHeight() * calibration.strumZoneBottom;

    if (pos.x >= szLeft && pos.x <= szRight && pos.y >= szTop && pos.y <= szBottom)
    {
        int stringIdx = static_cast<int>(
            (pos.y - szTop) / (szBottom - szTop) * 5.0f + 0.5f);
        stringIdx = std::max(0, std::min(5, stringIdx));
        int fret = lastState.fretState.fret;
        int midiNote = calibration.midiNoteForString(stringIdx, fret);
        NoteEvent evt;
        evt.type = NoteEventType::NoteOn;
        evt.midiNote = midiNote;
        evt.stringIndex = stringIdx;
        evt.fret = fret;
        evt.velocity = 0.7f;
        evt.timestampMs = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
        if (audioEngine)
            audioEngine->handleNoteEvent(evt);
        if (midiOutput && midiConnected)
            midiOutput->processNoteEvent(evt);
        mouseNoteString = stringIdx;
    }
}

void MainComponent::mouseUp(const juce::MouseEvent&)
{
    if (mouseNoteString >= 0)
    {
        NoteEvent evt;
        evt.type = NoteEventType::NoteOff;
        evt.stringIndex = mouseNoteString;
        evt.midiNote = -1;
        evt.velocity = 0.0f;
        evt.timestampMs = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
        if (audioEngine)
            audioEngine->handleNoteEvent(evt);
        if (midiOutput && midiConnected)
            midiOutput->processNoteEvent(evt);
        mouseNoteString = -1;
    }
}

void MainComponent::mouseMove(const juce::MouseEvent& event)
{
    auto pos = event.getPosition().toFloat();
    auto cam = getCameraArea();
    float szLeft = cam.getX() + cam.getWidth() * calibration.strumZoneLeft;
    float szRight = cam.getX() + cam.getWidth() * calibration.strumZoneRight;
    float szTop = cam.getY() + cam.getHeight() * calibration.strumZoneTop;
    float szBottom = cam.getY() + cam.getHeight() * calibration.strumZoneBottom;

    bool wasInZone = mouseInStrumZone;
    mouseInStrumZone = (pos.x >= szLeft && pos.x <= szRight
                     && pos.y >= szTop && pos.y <= szBottom);

    if (mouseInStrumZone != wasInZone)
        setMouseCursor(mouseInStrumZone
            ? juce::MouseCursor::PointingHandCursor
            : juce::MouseCursor::NormalCursor);
}

void MainComponent::mouseDrag(const juce::MouseEvent& event)
{
    mouseMove(event);
}

void MainComponent::drawNoteDisplay(juce::Graphics& g, const PhysicsState& state)
{
    if (noteOpacity < 0.01f || !state.tracking)
        return;

    auto bounds = getLocalBounds().toFloat();
    auto cam = getCameraArea();
    float alpha = noteOpacity * 0.45f;
    float glow = 0.5f + 0.5f * std::sin(glowPhase);
    float glowAlpha = alpha * (0.7f + 0.3f * glow);

    float fretLeft = cam.getX() + cam.getWidth() * calibration.fret1X;
    float fretRight = cam.getX() + cam.getWidth() * calibration.fret12X;
    float fretTop = cam.getY() + cam.getHeight() * calibration.stringTopY;
    float panelW = 140.0f;
    float panelH = 140.0f;
    float noteX = (fretLeft + fretRight) / 2.0f - panelW / 2.0f;
    float noteY = fretTop - panelH - 10.0f;

    noteX = std::max(5.0f, std::min(noteX, bounds.getWidth() - panelW - 5.0f));
    noteY = std::max(5.0f, noteY);

    g.setColour(juce::Colours::cyan.withAlpha(glowAlpha * 0.15f));
    g.fillRoundedRectangle(noteX, noteY, panelW, panelH, 10.0f);

    g.setFont(juce::Font(juce::FontOptions(36.0f, juce::Font::bold)));
    g.setColour(juce::Colours::cyan.withAlpha(glowAlpha));
    g.drawText(state.chordName, noteX, noteY, 140, 45,
               juce::Justification::centred);

    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.setColour(juce::Colours::white.withAlpha(alpha * 0.8f));
    g.drawText("Fret " + juce::String(state.fretState.fret),
               noteX, noteY + 42, 140, 18,
               juce::Justification::centred);

    static const char* stringNames[] = {"E", "A", "D", "G", "B", "e"};
    for (int i = 0; i < 6; ++i)
    {
        float y = noteY + 68 + static_cast<float>(i) * 17.0f;
        bool active = state.fretState.activeStrings[i] >= 0;

        float noteGlow = active ? glowAlpha : alpha * 0.3f;
        juce::Colour col = active ? juce::Colours::limegreen : juce::Colours::grey;
        g.setColour(col.withAlpha(noteGlow));

        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        g.drawText(juce::String(stringNames[i]), noteX + 5, y, 20, 15,
                   juce::Justification::centredLeft);

        if (active)
        {
            int midiNote = calibration.midiNoteForString(i, state.fretState.fret);
            std::string name = ChordClassifier::noteName(midiNote);
            g.setColour(juce::Colours::white.withAlpha(noteGlow));
            g.drawText(name, noteX + 28, y, 30, 15,
                       juce::Justification::centredLeft);
        }
    }

    if (state.strumming)
    {
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.setColour(juce::Colours::yellow.withAlpha(glowAlpha));
        juce::String dir = state.strumDirection == StrumDirection::Down ? "DN" : "UP";
        g.drawText(dir + " " + juce::String(state.strumVelocity, 2),
                   noteX, noteY + panelH - 18, panelW, 15,
                   juce::Justification::centred);
    }
}

void MainComponent::drawHelpOverlay(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour(juce::Colours::black.withAlpha(0.75f));
    g.fillRect(bounds);

    auto center = bounds.getCentre();
    float boxW = 380.0f;
    float boxH = 320.0f;
    auto box = juce::Rectangle<float>(
        center.x - boxW / 2.0f, center.y - boxH / 2.0f,
        boxW, boxH);

    g.setColour(juce::Colours::darkgrey.withAlpha(0.95f));
    g.fillRoundedRectangle(box, 12.0f);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(22.0f)));
    g.drawText("Keyboard Shortcuts", box.reduced(20).removeFromTop(40),
               juce::Justification::centred);

    struct Shortcut { const char* key; const char* desc; };
    Shortcut shortcuts[] = {
        {"C", "Start calibration wizard"},
        {"F", "Toggle fun mode"},
        {"M", "Toggle MIDI output"},
        {"?", "Show/hide this help"},
        {"Esc", "Cancel / close"},
        {"Click", "Play note in strum zone"},
    };

    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    auto content = box.reduced(30, 0).withTrimmedTop(50);

    for (auto& s : shortcuts)
    {
        auto row = content.removeFromTop(28);
        auto keyArea = row.removeFromLeft(70);

        g.setColour(juce::Colours::cyan.withAlpha(0.9f));
        g.drawText(s.key, keyArea, juce::Justification::centredRight);

        g.setColour(juce::Colours::lightgrey);
        g.drawText(s.desc, row.reduced(5, 0),
                   juce::Justification::centredLeft);
    }
}

void MainComponent::drawFunModeIndicator(juce::Graphics& g)
{
    if (!funMode)
        return;

    auto bounds = getLocalBounds().toFloat();
    float x = bounds.getWidth() - 80.0f;
    float y = 10.0f;

    float glow = 0.7f + 0.3f * std::sin(glowPhase * 1.5f);

    g.setColour(juce::Colours::magenta.withAlpha(0.12f * glow));
    g.fillRoundedRectangle(x - 5, y - 2, 75, 22, 6.0f);

    g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    g.setColour(juce::Colours::magenta.withAlpha(0.8f * glow));
    g.drawText("FUN", x, y, 65, 18, juce::Justification::centred);
}

} // namespace AirGuitar
