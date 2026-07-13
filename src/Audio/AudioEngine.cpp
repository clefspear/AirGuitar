#include "AudioEngine.h"

namespace AirGuitar {

AudioEngine::AudioEngine()
{
    setAudioChannels(0, 2);
}

AudioEngine::~AudioEngine()
{
    shutdownAudio();
}

void AudioEngine::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    stringModel.init(static_cast<int>(sampleRate));
    prepared = true;
}

void AudioEngine::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        while (!eventQueue.empty())
        {
            auto& evt = eventQueue.front();
            if (evt.type == NoteEventType::NoteOn)
                stringModel.noteOn(evt.stringIndex, evt.fret, evt.velocity,
                                   CalibrationData::defaultConfig());
            else if (evt.type == NoteEventType::NoteOff)
                stringModel.noteOff(evt.stringIndex);
            eventQueue.pop();
        }
    }

    auto* leftChannel = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
    auto* rightChannel = bufferToFill.buffer->getNumChannels() > 1
        ? bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample)
        : nullptr;

    for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
    {
        float output = stringModel.mix();
        leftChannel[sample] = output;
        if (rightChannel)
            rightChannel[sample] = output;
    }
}

void AudioEngine::releaseResources()
{
    prepared = false;
}

void AudioEngine::handleNoteEvent(const NoteEvent& evt)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    eventQueue.push(evt);
}

void AudioEngine::setMasterVolume(float vol)
{
    masterVolume = vol;
    stringModel.setMasterVolume(vol);
}

} // namespace AirGuitar
