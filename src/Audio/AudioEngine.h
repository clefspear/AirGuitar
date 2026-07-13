#pragma once
#include "StringModel.h"
#include "Physics/NoteEvent.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <mutex>
#include <queue>

namespace AirGuitar {

class AudioEngine : public juce::AudioIODeviceCallback
{
public:
    AudioEngine();
    ~AudioEngine() override;

    void startAudio();

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;

    void handleNoteEvent(const NoteEvent& evt);
    void setMasterVolume(float vol);

    bool isInitialised() const { return prepared; }

private:
    juce::AudioDeviceManager deviceManager;
    StringModel stringModel;
    std::queue<NoteEvent> eventQueue;
    std::mutex queueMutex;
    bool prepared = false;
    bool audioStarted = false;
    float masterVolume = 0.7f;
};

} // namespace AirGuitar
