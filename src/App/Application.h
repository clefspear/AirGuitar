#pragma once
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace AirGuitar {

class MainComponent;

class Application : public juce::JUCEApplication
{
public:
    Application() = default;

    const juce::String getApplicationName() override { return "AirGuitar"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String&) override;
    void shutdown() override;
    void systemRequestedQuit() override;
    void anotherInstanceStarted(const juce::String&) override {}

private:
    std::unique_ptr<juce::DocumentWindow> mainWindow;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Application)
};

} // namespace AirGuitar
