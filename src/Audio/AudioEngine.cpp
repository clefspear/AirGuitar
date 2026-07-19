#include "AudioEngine.h"
#include "App/CrashLogger.h"
#include <sstream>

namespace AirGuitar {

AudioEngine::AudioEngine()
{
}

AudioEngine::~AudioEngine()
{
    if (audioStarted)
        deviceManager.removeAudioCallback(this);
}

void AudioEngine::startAudio()
{
    if (audioStarted)
        return;

    auto error = deviceManager.initialiseWithDefaultDevices(0, 2);
    if (error.isNotEmpty())
    {
        startError = error.toStdString();
        return;
    }

    deviceManager.addAudioCallback(this);
    audioStarted = true;
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device != nullptr)
        stringModel.init(static_cast<int>(device->getCurrentSampleRate()));
    prepared = true;
}

void AudioEngine::audioDeviceStopped()
{
    prepared = false;
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                    int numInputChannels,
                                                    float* const* outputChannelData,
                                                    int numOutputChannels,
                                                    int numSamples,
                                                    const juce::AudioIODeviceCallbackContext& context)
{
    for (int ch = 0; ch < numOutputChannels; ++ch)
        juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);

    int queueSize = 0;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        queueSize = static_cast<int>(eventQueue.size());
        while (!eventQueue.empty())
        {
            auto& evt = eventQueue.front();
            if (evt.type == NoteEventType::NoteOn)
                stringModel.noteOn(evt.stringIndex, evt.midiNote, evt.velocity);
            else if (evt.type == NoteEventType::NoteOff)
                stringModel.noteOff(evt.stringIndex);
            eventQueue.pop();
        }
    }

    static int audioLogCounter = 0;
    if (++audioLogCounter % 48000 == 0)
    {
        std::ostringstream ss;
        ss << "[Audio] callback prepared=" << prepared
           << " queue=" << queueSize
           << " active=" << stringModel.isAnyActive();
        CrashLogger::instance().logInfo(ss.str());
    }

    if (numOutputChannels > 0)
    {
        auto* leftChannel = outputChannelData[0];
        for (int sample = 0; sample < numSamples; ++sample)
            leftChannel[sample] = stringModel.mix();

        if (numOutputChannels > 1)
            juce::FloatVectorOperations::copyWithMultiply(
                outputChannelData[1], leftChannel, 1.0f, numSamples);
    }
}

void AudioEngine::handleNoteEvent(const NoteEvent& evt)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    eventQueue.push(evt);

    std::ostringstream ss;
    ss << "[Audio] event=" << (evt.type == NoteEventType::NoteOn ? "ON" : "OFF")
       << " string=" << evt.stringIndex
       << " midi=" << evt.midiNote
       << " vel=" << evt.velocity;
    CrashLogger::instance().logInfo(ss.str());
}

void AudioEngine::setMasterVolume(float vol)
{
    masterVolume = vol;
    stringModel.setMasterVolume(vol);
}

} // namespace AirGuitar
