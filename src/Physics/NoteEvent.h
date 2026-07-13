#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace AirGuitar {

enum class NoteEventType
{
    NoteOn,
    NoteOff,
    StrumStart,
    StrumEnd
};

enum class StrumDirection
{
    Down,
    Up,
    None
};

struct NoteEvent
{
    NoteEventType type = NoteEventType::NoteOn;
    int midiNote = 0;
    int stringIndex = 0;
    int fret = 0;
    float velocity = 0.0f;
    StrumDirection direction = StrumDirection::None;
    int64_t timestampMs = 0;
};

struct FretState
{
    int fret = 0;
    int activeStrings[6] = {-1, -1, -1, -1, -1, -1};
    bool fingerExtended[5] = {false, false, false, false, false};
};

struct PhysicsState
{
    FretState fretState;
    std::string chordName;
    std::vector<NoteEvent> events;
    float strumVelocity = 0.0f;
    StrumDirection strumDirection = StrumDirection::None;
    bool strumming = false;
    bool tracking = false;
    int64_t timestampMs = 0;
};

} // namespace AirGuitar
