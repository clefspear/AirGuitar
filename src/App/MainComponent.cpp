#include "MainComponent.h"
#include "Application.h"
#include "CrashLogger.h"
#include <opencv2/imgcodecs.hpp>

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
    physicsEngine->setNoteCallback([this](const NoteEvent& evt) {
        if (audioEngine)
            audioEngine->handleNoteEvent(evt);
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

                camera->setFrameCallback(
                    [this](const cv::Mat& frame, int64_t timestampMs)
                    {
                        if (handPipeline && handPipeline->isRunning())
                            handPipeline->onFrameReceived(frame, timestampMs);
                    });

                camera->startCapture();
                CrashLogger::instance().logInfo("Hand pipeline started, camera capture running");
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
                break;
            default:
                initError = InitError::CameraOpenFailed;
                initErrorMessage = "Camera failed to open (error " +
                    std::to_string(static_cast<int>(result)) + ")";
                break;
        }
        CrashLogger::instance().logError("Init", initErrorMessage);
    }

    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    stopTimer();
    CrashLogger::instance().logInfo("Shutting down");
    if (physicsEngine) physicsEngine->stop();
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
            for (const auto& hand : landmarks.hands)
            {
                juce::Colour colour = hand.isLeft
                    ? juce::Colours::cyan
                    : juce::Colours::orange;
                drawLandmarks(g, hand, colour);
            }
            drawPose(g, landmarks.pose);

            if (physicsEngine)
            {
                physicsEngine->pushFrame(landmarks);
            }
        }
    }

    if (physicsEngine && cameraReady)
    {
        drawStrumZone(g);
        drawChordOverlay(g, physicsEngine->getLatestState());
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
    if (frameCount % 30 == 0)
    {
        if (handPipeline) inferenceFps = handPipeline->getInferenceRate();
        if (camera) captureFps = camera->getFrameRate();
    }
    repaint();
}

void MainComponent::handleMessage(const juce::Message&)
{
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
        }
    }
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
}

void MainComponent::drawLandmarks(juce::Graphics& g,
                                   const HandLandmarks& hand,
                                   const juce::Colour& colour)
{
    if (hand.landmarks.size() != 21)
        return;

    auto bounds = getLocalBounds().toFloat();

    g.setColour(colour);

    for (size_t i = 0; i < hand.landmarks.size(); ++i)
    {
        auto& lm = hand.landmarks[i];
        auto px = lm.x * bounds.getWidth();
        auto py = lm.y * bounds.getHeight();
        g.fillEllipse(px - 3.0f, py - 3.0f, 6.0f, 6.0f);
    }

    static const std::vector<std::pair<int, int>> connections = {
        {0, 1}, {1, 2}, {2, 3}, {3, 4},
        {0, 5}, {5, 6}, {6, 7}, {7, 8},
        {0, 9}, {9, 10}, {10, 11}, {11, 12},
        {0, 13}, {13, 14}, {14, 15}, {15, 16},
        {0, 17}, {17, 18}, {18, 19}, {19, 20}
    };

    for (auto [a, b] : connections)
    {
        auto& p1 = hand.landmarks[a];
        auto& p2 = hand.landmarks[b];
        g.drawLine(p1.x * bounds.getWidth(), p1.y * bounds.getHeight(),
                   p2.x * bounds.getWidth(), p2.y * bounds.getHeight(),
                   2.0f);
    }
}

void MainComponent::drawPose(juce::Graphics& g, const PoseLandmarks& pose)
{
    if (pose.landmarks.size() < 25)
        return;

    auto bounds = getLocalBounds().toFloat();

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

        g.drawLine(p1.x * bounds.getWidth(), p1.y * bounds.getHeight(),
                   p2.x * bounds.getWidth(), p2.y * bounds.getHeight(),
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

void MainComponent::drawStrumZone(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    float left = bounds.getWidth() * calibration.strumZoneLeft;
    float right = bounds.getWidth() * calibration.strumZoneRight;
    float top = bounds.getHeight() * calibration.strumZoneTop;
    float bottom = bounds.getHeight() * calibration.strumZoneBottom;

    g.setColour(juce::Colours::yellow.withAlpha(0.15f));
    g.fillRect(left, top, right - left, bottom - top);

    g.setColour(juce::Colours::yellow.withAlpha(0.4f));
    g.drawRect(left, top, right - left, bottom - top, 1.0f);

    g.setColour(juce::Colours::yellow.withAlpha(0.3f));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));

    for (int i = 0; i < 6; ++i)
    {
        float y = top + static_cast<float>(i) / 5.0f * (bottom - top);
        g.drawLine(left, y, right, y, 0.5f);
        g.drawText("S" + juce::String(i + 1), right + 5, y - 6, 25, 12,
                   juce::Justification::left);
    }
}

} // namespace AirGuitar
