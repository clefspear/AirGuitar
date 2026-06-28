#include "Application.h"
#include "MainComponent.h"

namespace AirGuitar {

class MainWindow : public juce::DocumentWindow
{
public:
    explicit MainWindow()
        : DocumentWindow("AirGuitar",
                         juce::Desktop::getInstance()
                             .getDefaultLookAndFeel()
                             .findColour(juce::ResizableWindow::backgroundColourId),
                         allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new MainComponent(), true);
        setResizable(true, false);

        centreWithSize(1280, 720);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

void Application::initialise(const juce::String&)
{
    mainWindow = std::make_unique<MainWindow>();
}

void Application::shutdown()
{
    mainWindow = nullptr;
}

void Application::systemRequestedQuit()
{
    juce::MessageManager::getInstance()->runDispatchLoopUntil(250);
    quit();
}

} // namespace AirGuitar
