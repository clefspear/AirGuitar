#include "ChordClassifier.h"
#include <algorithm>
#include <set>

namespace AirGuitar {

static const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

ChordClassifier::ChordClassifier() {}

const std::vector<ChordClassifier::ChordFormula>& ChordClassifier::formulas()
{
    static const std::vector<ChordFormula> f = {
        {"",       {0, 4, 7}},
        {"m",      {0, 3, 7}},
        {"7",      {0, 4, 7, 10}},
        {"maj7",   {0, 4, 7, 11}},
        {"m7",     {0, 3, 7, 10}},
        {"dim",    {0, 3, 6}},
        {"aug",    {0, 4, 8}},
        {"sus2",   {0, 2, 7}},
        {"sus4",   {0, 5, 7}},
        {"add9",   {0, 4, 7, 14}},
        {"9",      {0, 4, 7, 10, 14}},
        {"m9",     {0, 3, 7, 10, 14}},
        {"6",      {0, 4, 7, 9}},
        {"m6",     {0, 3, 7, 9}},
        {"dim7",   {0, 3, 6, 9}},
        {"7#5",    {0, 4, 8, 10}},
        {"7b5",    {0, 4, 6, 10}},
        {"m7b5",   {0, 3, 6, 10}},
        {"11",     {0, 4, 7, 10, 17}},
        {"13",     {0, 4, 7, 10, 21}},
        {"m11",    {0, 3, 7, 10, 17}},
        {"aug7",   {0, 4, 8, 10}},
        {"7sus4",  {0, 5, 7, 10}},
        {"7sus2",  {0, 2, 7, 10}},
        {"maj9",   {0, 4, 7, 11, 14}},
        {"5",      {0, 7}},
    };
    return f;
}

ChordMatch ChordClassifier::classify(int currentFret, const int activeStrings[6],
                                     const CalibrationData& cal)
{
    std::vector<int> midiNotes;
    for (int s = 0; s < cal.kNumStrings; ++s)
    {
        if (activeStrings[s] >= 0)
        {
            int note = cal.midiNoteForString(s, activeStrings[s]);
            if (note >= 0)
                midiNotes.push_back(note);
        }
    }
    return classifyFromNotes(midiNotes);
}

ChordMatch ChordClassifier::classifyFromNotes(const std::vector<int>& midiNotes)
{
    if (midiNotes.empty())
        return ChordMatch{"", 0, false};

    std::set<int> pitchClasses;
    for (int n : midiNotes)
        pitchClasses.insert(noteToPitchClass(n));

    std::vector<int> pcs(pitchClasses.begin(), pitchClasses.end());

    if (pcs.size() == 1)
    {
        return ChordMatch{noteName(pcs[0]), pcs[0], true};
    }

    const auto& ch = formulas();

    ChordMatch bestMatch;
    int bestScore = -1;

    for (int root : pcs)
    {
        std::vector<int> intervals;
        for (int pc : pcs)
        {
            int interval = (pc - root + 12) % 12;
            intervals.push_back(interval);
        }
        std::sort(intervals.begin(), intervals.end());

        for (const auto& formula : ch)
        {
            std::set<int> formulaPCs;
            for (int interval : formula.intervals)
                formulaPCs.insert(interval % 12);

            if (formulaPCs.size() != intervals.size())
                continue;

            std::vector<int> sortedFormula(formulaPCs.begin(), formulaPCs.end());
            std::sort(sortedFormula.begin(), sortedFormula.end());

            bool match = true;
            for (size_t i = 0; i < intervals.size(); ++i)
            {
                if (intervals[i] != sortedFormula[i])
                {
                    match = false;
                    break;
                }
            }

            if (match)
            {
                int score = static_cast<int>(intervals.size());
                if (score > bestScore)
                {
                    bestScore = score;
                    std::string name = noteName(root);
                    name += formula.suffix;
                    bestMatch = ChordMatch{name, root, true};
                }
            }
        }
    }

    if (bestMatch.valid)
        return bestMatch;

    std::string fallback;
    for (size_t i = 0; i < pcs.size(); ++i)
    {
        if (i > 0) fallback += "/";
        fallback += noteName(pcs[i]);
    }
    return ChordMatch{fallback, pcs[0], true};
}

std::string ChordClassifier::noteName(int midiNote)
{
    int pc = noteToPitchClass(midiNote);
    return noteNames[pc];
}

int ChordClassifier::noteToPitchClass(int midiNote)
{
    return ((midiNote % 12) + 12) % 12;
}

} // namespace AirGuitar
