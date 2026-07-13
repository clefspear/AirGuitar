#pragma once
#include <string>
#include <vector>
#include "CalibrationData.h"

namespace AirGuitar {

struct ChordMatch
{
    std::string name;
    int rootNote = 0;
    bool valid = false;
};

class ChordClassifier
{
public:
    ChordClassifier();

    ChordMatch classify(int currentFret, const int activeStrings[6],
                        const CalibrationData& cal);

    ChordMatch classifyFromNotes(const std::vector<int>& midiNotes);

    static std::string noteName(int midiNote);
    static int noteToPitchClass(int midiNote);

private:
    struct ChordFormula
    {
        const char* suffix;
        std::vector<int> intervals;
    };

    static const std::vector<ChordFormula>& formulas();

    static ChordMatch matchFormula(int root, const std::vector<int>& intervals,
                                   const ChordFormula& formula);
};

} // namespace AirGuitar
