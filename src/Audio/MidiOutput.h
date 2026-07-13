#pragma once
#include "Physics/NoteEvent.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <string>
#include <vector>

namespace AirGuitar {

class AirGuitarMidiOutput
{
public:
    AirGuitarMidiOutput();
    ~AirGuitarMidiOutput();

    bool openDevice(const std::string& deviceName);
    void closeDevice();

    void sendNoteOn(int channel, int note, int velocity);
    void sendNoteOff(int channel, int note);
    void sendCC(int channel, int controller, int value);

    void processNoteEvent(const NoteEvent& evt);

    bool isConnected() const;
    std::string getDeviceName() const;

    static std::vector<std::string> getAvailableDevices();

private:
    juce::MidiOutput* rawOutput = nullptr;
    std::string currentDeviceName;
};

} // namespace AirGuitar
