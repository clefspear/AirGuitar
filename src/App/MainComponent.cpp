#include "MainComponent.h"
#include "Application.h"

namespace AirGuitar {

MainComponent::MainComponent()
{
    setSize(1280, 720);

    camera = std::make_unique<Camera>();

    auto result = camera->open(0, Camera::kDefaultWidth,
                                Camera::kDefaultHeight,
                                Camera::kDefaultFps);

    if (result == Camera::Error::None)
    {
        handPipeline = std::make_unique<HandPipeline>();

        auto modelsDir = juce::File::getSpecialLocation(
            juce::File::currentExecutableFile).getParentDirectory()
            .getChildFile("../../models");

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
        }
        else
        {
            DBG("HandPipeline init failed: " << static_cast<int>(loadResult));
        }
    }
    else
    {
        DBG("Camera open failed: " << static_cast<int>(result));
    }

    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    stopTimer();
    if (handPipeline) handPipeline->stop();
    if (camera) camera->stopCapture();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    if (cameraReady && camera && handPipeline)
    {
        auto latestFrame = camera->getLatestFrame();
        if (latestFrame.data != nullptr)
        {
            auto img = juce::Image(juce::Image::PixelFormat::RGB,
                                   latestFrame.cols, latestFrame.rows, false);

            juce::Image::BitmapData bitmapData(img,
                juce::Image::BitmapData::writeOnly);

            for (int y = 0; y < latestFrame.rows; ++y)
            {
                auto srcRow = latestFrame.data + y * latestFrame.step;
                auto dstRow = bitmapData.getLinePointer(y);
                std::copy(srcRow, srcRow + latestFrame.cols * 3, dstRow);
            }

            auto bounds = getLocalBounds().toFloat();
            auto scaleX = bounds.getWidth() / static_cast<float>(latestFrame.cols);
            auto scaleY = bounds.getHeight() / static_cast<float>(latestFrame.rows);
            auto scale = std::min(scaleX, scaleY);

            auto w = latestFrame.cols * scale;
            auto h = latestFrame.rows * scale;
            auto x = (bounds.getWidth() - w) / 2.0f;
            auto yPos = (bounds.getHeight() - h) / 2.0f;

            g.drawImage(img, x, yPos, w, h, 0, 0,
                        latestFrame.cols, latestFrame.rows);

            auto landmarks = handPipeline->getLatestLandmarks();
            for (const auto& hand : landmarks.hands)
            {
                juce::Colour colour = hand.isLeft
                    ? juce::Colours::cyan
                    : juce::Colours::orange;
                drawLandmarks(g, hand, colour);
            }
            drawPose(g, landmarks.pose);
        }
    }

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

void MainComponent::drawDebugOverlay(juce::Graphics& g)
{
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::Font(14.0f));

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
    else
    {
        g.drawText("Camera not available", x, y, 200, lineH,
                   juce::Justification::left);
    }
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

} // namespace AirGuitar
