#include "CalibrationWizard.h"
#include "Physics/ChordClassifier.h"
#include <algorithm>
#include <cmath>

namespace AirGuitar {

CalibrationWizard::CalibrationWizard()
{
    setWantsKeyboardFocus(true);
    result = CalibrationData::defaultConfig();
}

CalibrationWizard::~CalibrationWizard()
{
    stopTimer();
}

void CalibrationWizard::start()
{
    step = Step::Welcome;
    completed = false;
    dwellFrames = 0;
    result = CalibrationData::defaultConfig();
    grabKeyboardFocus();
    startTimerHz(30);
}

void CalibrationWizard::cancel()
{
    step = Step::None;
    completed = false;
    stopTimer();
}

void CalibrationWizard::setCompleteCallback(CompleteCallback cb)
{
    onComplete = std::move(cb);
}

void CalibrationWizard::feedLandmarks(const FrameData& landmarks)
{
    latestLandmarks = landmarks;
}

void CalibrationWizard::timerCallback()
{
    if (step == Step::Fret1 || step == Step::Fret12 || step == Step::StrumZone)
    {
        if (!latestLandmarks.hands.empty())
        {
            ++dwellFrames;
            if (dwellFrames >= kDwellRequired)
            {
                advanceStep();
            }
        }
        else
        {
            dwellFrames = std::max(0, dwellFrames - 2);
        }
    }
    repaint();
}

bool CalibrationWizard::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        cancel();
        return true;
    }

    if (step == Step::Welcome || step == Step::Complete)
    {
        advanceStep();
    }
    return true;
}

void CalibrationWizard::advanceStep()
{
    int currentStep = static_cast<int>(step);

    if (step == Step::Fret1 && !latestLandmarks.hands.empty())
    {
        float sumX = 0.0f;
        int count = 0;
        for (auto& hand : latestLandmarks.hands)
        {
            if (hand.valid() && hand.landmarks.size() >= 21)
            {
                for (int i = 0; i < 21; ++i)
                    sumX += hand.landmarks[i].x;
                count += 21;
            }
        }
        if (count > 0)
            fret1X = sumX / static_cast<float>(count);
    }
    else if (step == Step::Fret12 && !latestLandmarks.hands.empty())
    {
        float sumX = 0.0f;
        int count = 0;
        for (auto& hand : latestLandmarks.hands)
        {
            if (hand.valid() && hand.landmarks.size() >= 21)
            {
                for (int i = 0; i < 21; ++i)
                    sumX += hand.landmarks[i].x;
                count += 21;
            }
        }
        if (count > 0)
            fret12X = sumX / static_cast<float>(count);
    }
    else if (step == Step::StrumZone && !latestLandmarks.hands.empty())
    {
        float minX = 1.0f, maxX = 0.0f;
        float minY = 1.0f, maxY = 0.0f;
        for (auto& hand : latestLandmarks.hands)
        {
            if (hand.valid() && hand.landmarks.size() >= 21)
            {
                for (auto& lm : hand.landmarks)
                {
                    minX = std::min(minX, lm.x);
                    maxX = std::max(maxX, lm.x);
                    minY = std::min(minY, lm.y);
                    maxY = std::max(maxY, lm.y);
                }
            }
        }
        strumLeft = minX;
        strumRight = maxX;
        strumTop = minY;
        strumBottom = maxY;
    }

    ++currentStep;
    step = static_cast<Step>(currentStep);
    dwellFrames = 0;

    if (step == Step::Complete)
    {
        computeCalibration();
        completed = true;
        stopTimer();
        repaint();
        if (onComplete)
            onComplete(result);
    }
}

void CalibrationWizard::computeCalibration()
{
    float left = std::min(fret1X, fret12X);
    float right = std::max(fret1X, fret12X);
    float range = right - left;

    if (range < 0.05f)
    {
        result = CalibrationData::defaultConfig();
        return;
    }

    result.fretOriginX = left;
    result.fret1X = left;
    result.fret12X = right;
    result.fretScaleX = range / 11.0f;

    float margin = 0.02f;
    result.strumZoneLeft = std::max(0.0f, strumLeft - margin);
    result.strumZoneRight = std::min(1.0f, strumRight + margin);
    result.strumZoneTop = std::max(0.0f, strumTop - margin);
    result.strumZoneBottom = std::min(1.0f, strumBottom + margin);

    result.stringTopY = result.strumZoneTop;
    result.stringBottomY = result.strumZoneBottom;
}

void CalibrationWizard::paint(juce::Graphics& g)
{
    if (step == Step::None)
        return;

    auto bounds = getLocalBounds().toFloat();

    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRect(bounds);

    auto center = bounds.getCentre();
    float boxW = 500.0f;
    float boxH = 260.0f;
    auto box = juce::Rectangle<float>(
        center.x - boxW / 2.0f, center.y - boxH / 2.0f,
        boxW, boxH);

    g.setColour(juce::Colours::darkgrey.withAlpha(0.9f));
    g.fillRoundedRectangle(box, 12.0f);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(28.0f)));

    std::string title;
    std::string instruction;
    std::string hint;

    switch (step)
    {
        case Step::Welcome:
            title = "Calibration Wizard";
            instruction = "Press any key to begin";
            hint = "You'll be asked to show hand positions";
            break;
        case Step::Fret1:
            title = "Step 1: Fret 1";
            instruction = "Show your fret hand near fret 1";
            hint = "Hold still for 1 second...";
            break;
        case Step::Fret12:
            title = "Step 2: Fret 12";
            instruction = "Show your fret hand near fret 12";
            hint = "Hold still for 1 second...";
            break;
        case Step::StrumZone:
            title = "Step 3: Strum Zone";
            instruction = "Show your strum hand in the strum zone";
            hint = "Hold still for 1 second...";
            break;
        case Step::Complete:
            title = "Calibration Complete!";
            instruction = "Press any key to start playing";
            hint = "";
            break;
        default:
            break;
    }

    g.drawText(title, box.reduced(20).removeFromTop(50),
               juce::Justification::centred);

    g.setFont(juce::Font(juce::FontOptions(18.0f)));
    g.setColour(juce::Colours::lightgrey);
    g.drawText(instruction, box.reduced(20).removeFromTop(40),
               juce::Justification::centred);

    if (!hint.empty())
    {
        g.setFont(juce::Font(juce::FontOptions(14.0f)));
        g.setColour(juce::Colours::grey);
        g.drawText(hint, box.reduced(20).removeFromTop(30),
                   juce::Justification::centred);
    }

    if (dwellFrames > 0 && step != Step::Welcome && step != Step::Complete)
    {
        float progress = static_cast<float>(dwellFrames)
                       / static_cast<float>(kDwellRequired);
        auto progressBounds = box.reduced(40).removeFromBottom(10);
        g.setColour(juce::Colours::grey.withAlpha(0.3f));
        g.fillRoundedRectangle(progressBounds, 4.0f);
        g.setColour(juce::Colours::limegreen);
        g.fillRoundedRectangle(
            progressBounds.withWidth(progressBounds.getWidth() * progress), 4.0f);
    }

    if (step == Step::Fret1 || step == Step::Fret12)
    {
        int fretNum = (step == Step::Fret1) ? 1 : 12;
        auto noteArea = box.withTop(box.getBottom() - 70).reduced(30, 5);

        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        g.setColour(juce::Colours::cyan.withAlpha(0.7f));
        g.drawText("Fret " + juce::String(fretNum) + " notes:",
                   noteArea.removeFromTop(16),
                   juce::Justification::centredLeft);

        struct StringInfo { const char* name; int openMidi; };
        StringInfo strings[] = {
            {"6th", 40}, {"5th", 45}, {"4th", 50},
            {"3rd", 55}, {"2nd", 59}, {"1st", 64}
        };

        g.setFont(juce::Font(juce::FontOptions(13.0f)));
        float colW = noteArea.getWidth() / 6.0f;
        for (int i = 0; i < 6; ++i)
        {
            int midiNote = strings[i].openMidi + fretNum;
            std::string name = ChordClassifier::noteName(midiNote);
            float x = noteArea.getX() + static_cast<float>(i) * colW;

            g.setColour(juce::Colours::limegreen.withAlpha(0.6f));
            g.drawText(strings[i].name, x, noteArea.getY(), colW, 15,
                       juce::Justification::centred);

            g.setColour(juce::Colours::white.withAlpha(0.85f));
            g.drawText(name, x, noteArea.getY() + 14, colW, 16,
                       juce::Justification::centred);
        }
    }

    if (step == Step::StrumZone)
    {
        auto noteArea = box.withTop(box.getBottom() - 65).reduced(30, 5);

        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        g.setColour(juce::Colours::cyan.withAlpha(0.7f));
        g.drawText("Open string notes:",
                   noteArea.removeFromTop(16),
                   juce::Justification::centredLeft);

        struct StringInfo { const char* name; int openMidi; };
        StringInfo strings[] = {
            {"6th", 40}, {"5th", 45}, {"4th", 50},
            {"3rd", 55}, {"2nd", 59}, {"1st", 64}
        };

        g.setFont(juce::Font(juce::FontOptions(13.0f)));
        float colW = noteArea.getWidth() / 6.0f;
        for (int i = 0; i < 6; ++i)
        {
            std::string name = ChordClassifier::noteName(strings[i].openMidi);
            float x = noteArea.getX() + static_cast<float>(i) * colW;

            g.setColour(juce::Colours::limegreen.withAlpha(0.6f));
            g.drawText(strings[i].name, x, noteArea.getY(), colW, 15,
                       juce::Justification::centred);

            g.setColour(juce::Colours::white.withAlpha(0.85f));
            g.drawText(name, x, noteArea.getY() + 14, colW, 16,
                       juce::Justification::centred);
        }
    }

    if (step == Step::Complete)
    {
        g.setColour(juce::Colours::limegreen);
        g.setFont(juce::Font(juce::FontOptions(16.0f)));
        g.drawText("Fret 1 X: " + juce::String(fret1X, 3) +
                   "  Fret 12 X: " + juce::String(fret12X, 3),
                   box.reduced(20).removeFromBottom(30),
                   juce::Justification::centred);
    }
}

void CalibrationWizard::resized()
{
}

} // namespace AirGuitar
