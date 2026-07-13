#pragma once
#include "StringModel.h"
#include "Physics/NoteEvent.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <mutex>
#include <queue>

namespace AirGuitar {

class AudioEngine : public juce::AudioAppComponent
{
public:
    AudioEngine();
    ~AudioEngine() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void handleNoteEvent(const NoteEvent& evt);
    void setMasterVolume(float vol);

    bool isInitialised() const { return prepared; }

private:
    StringModel stringModel;
    std::queue<NoteEvent> eventQueue;
    std::mutex queueMutex;
    bool prepared = false;
    float masterVolume = 0.7f;
};

} // namespace AirGuitar
