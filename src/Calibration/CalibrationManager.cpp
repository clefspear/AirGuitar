#include "CalibrationManager.h"
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace AirGuitar {

CalibrationManager::CalibrationManager()
{
    calibrationPath = getDefaultPath();
}

std::string CalibrationManager::getDefaultPath() const
{
    const char* home = std::getenv("HOME");
    if (!home)
        return "airguitar_calibration.txt";

    return std::string(home) + "/.airguitar_calibration";
}

bool CalibrationManager::hasSavedCalibration() const
{
    std::ifstream file(calibrationPath);
    return file.good();
}

bool CalibrationManager::save(const CalibrationData& cal, const std::string& path)
{
    std::string target = path.empty() ? calibrationPath : path;
    std::ofstream file(target);
    if (!file.is_open())
        return false;

    file << "fretOriginX=" << cal.fretOriginX << "\n";
    file << "fretScaleX=" << cal.fretScaleX << "\n";
    file << "fret1X=" << cal.fret1X << "\n";
    file << "fret12X=" << cal.fret12X << "\n";
    file << "stringTopY=" << cal.stringTopY << "\n";
    file << "stringBottomY=" << cal.stringBottomY << "\n";
    file << "strumZoneLeft=" << cal.strumZoneLeft << "\n";
    file << "strumZoneRight=" << cal.strumZoneRight << "\n";
    file << "strumZoneTop=" << cal.strumZoneTop << "\n";
    file << "strumZoneBottom=" << cal.strumZoneBottom << "\n";
    file << "extendedThreshold=" << cal.extendedThreshold << "\n";
    file << "referenceHandSize=" << cal.referenceHandSize << "\n";
    file << "leftHandFretting=" << (cal.leftHandFretting ? "1" : "0") << "\n";
    file << "funMode=" << (cal.funMode ? "1" : "0") << "\n";
    file << "minStrumSpeed=" << cal.minStrumSpeed << "\n";
    file << "maxStrumSpeed=" << cal.maxStrumSpeed << "\n";
    file << "strumCooldownMs=" << cal.strumCooldownMs << "\n";

    for (int i = 0; i < 6; ++i)
        file << "funModeChord" << i << "=" << cal.funModeChords[i] << "\n";

    return file.good();
}

bool CalibrationManager::load(CalibrationData& cal, const std::string& path)
{
    std::string target = path.empty() ? calibrationPath : path;
    std::ifstream file(target);
    if (!file.is_open())
        return false;

    std::string line;
    while (std::getline(file, line))
    {
        auto pos = line.find('=');
        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        if (key == "fretOriginX") cal.fretOriginX = std::stof(value);
        else if (key == "fretScaleX") cal.fretScaleX = std::stof(value);
        else if (key == "fret1X") cal.fret1X = std::stof(value);
        else if (key == "fret12X") cal.fret12X = std::stof(value);
        else if (key == "stringTopY") cal.stringTopY = std::stof(value);
        else if (key == "stringBottomY") cal.stringBottomY = std::stof(value);
        else if (key == "strumZoneLeft") cal.strumZoneLeft = std::stof(value);
        else if (key == "strumZoneRight") cal.strumZoneRight = std::stof(value);
        else if (key == "strumZoneTop") cal.strumZoneTop = std::stof(value);
        else if (key == "strumZoneBottom") cal.strumZoneBottom = std::stof(value);
        else if (key == "extendedThreshold") cal.extendedThreshold = std::stof(value);
        else if (key == "referenceHandSize") cal.referenceHandSize = std::stof(value);
        else if (key == "leftHandFretting") cal.leftHandFretting = (value == "1");
        else if (key == "funMode") cal.funMode = (value == "1");
        else if (key == "minStrumSpeed") cal.minStrumSpeed = std::stof(value);
        else if (key == "maxStrumSpeed") cal.maxStrumSpeed = std::stof(value);
        else if (key == "strumCooldownMs") cal.strumCooldownMs = std::stoi(value);
        else if (key.size() > 12 && key.substr(0, 12) == "funModeChord")
        {
            int idx = std::stoi(key.substr(12));
            if (idx >= 0 && idx < 6)
                cal.funModeChords[idx] = std::stoi(value);
        }
    }

    return true;
}

} // namespace AirGuitar
