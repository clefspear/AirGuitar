#include "MidiOutput.h"

namespace AirGuitar {

AirGuitarMidiOutput::AirGuitarMidiOutput() {}

AirGuitarMidiOutput::~AirGuitarMidiOutput()
{
    closeDevice();
}

bool AirGuitarMidiOutput::openDevice(const std::string& deviceName)
{
    closeDevice();

    auto devices = juce::MidiOutput::getAvailableDevices();

    for (int i = 0; i < devices.size(); ++i)
    {
        if (devices[i].name.toStdString() == deviceName)
        {
            auto output = juce::MidiOutput::openDevice(devices[i].identifier);
            if (output)
            {
                rawOutput = output.release();
                currentDeviceName = deviceName;
                return true;
            }
        }
    }
    return false;
}

void AirGuitarMidiOutput::closeDevice()
{
    if (rawOutput)
    {
        rawOutput->stopBackgroundThread();
        delete rawOutput;
        rawOutput = nullptr;
        currentDeviceName.clear();
    }
}

void AirGuitarMidiOutput::sendNoteOn(int channel, int note, int velocity)
{
    if (!rawOutput)
        return;

    channel = juce::jlimit(1, 16, channel);
    note = juce::jlimit(0, 127, note);
    velocity = juce::jlimit(0, 127, velocity);

    rawOutput->sendMessageNow(juce::MidiMessage::noteOn(channel, note,
                                                         static_cast<float>(velocity)));
}

void AirGuitarMidiOutput::sendNoteOff(int channel, int note)
{
    if (!rawOutput)
        return;

    channel = juce::jlimit(1, 16, channel);
    note = juce::jlimit(0, 127, note);

    rawOutput->sendMessageNow(juce::MidiMessage::noteOff(channel, note));
}

void AirGuitarMidiOutput::sendCC(int channel, int controller, int value)
{
    if (!rawOutput)
        return;

    channel = juce::jlimit(1, 16, channel);
    controller = juce::jlimit(0, 127, controller);
    value = juce::jlimit(0, 127, value);

    rawOutput->sendMessageNow(juce::MidiMessage::controllerEvent(channel, controller, value));
}

void AirGuitarMidiOutput::processNoteEvent(const NoteEvent& evt)
{
    int channel = 1;

    if (evt.type == NoteEventType::NoteOn)
    {
        sendNoteOn(channel, evt.midiNote, static_cast<int>(evt.velocity * 127));
        sendCC(channel, 11, static_cast<int>(evt.velocity * 127));
    }
    else if (evt.type == NoteEventType::NoteOff)
    {
        sendNoteOff(channel, evt.midiNote);
    }
}

bool AirGuitarMidiOutput::isConnected() const
{
    return rawOutput != nullptr;
}

std::string AirGuitarMidiOutput::getDeviceName() const
{
    return currentDeviceName;
}

std::vector<std::string> AirGuitarMidiOutput::getAvailableDevices()
{
    std::vector<std::string> result;
    auto devices = juce::MidiOutput::getAvailableDevices();
    for (auto& d : devices)
        result.push_back(d.name.toStdString());
    return result;
}

} // namespace AirGuitar
