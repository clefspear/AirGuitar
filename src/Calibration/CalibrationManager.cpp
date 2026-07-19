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

    CalibrationData loaded;
    std::string line;
    while (std::getline(file, line))
    {
        auto pos = line.find('=');
        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        if (key == "fretOriginX") loaded.fretOriginX = std::stof(value);
        else if (key == "fretScaleX") loaded.fretScaleX = std::stof(value);
        else if (key == "fret1X") loaded.fret1X = std::stof(value);
        else if (key == "fret12X") loaded.fret12X = std::stof(value);
        else if (key == "stringTopY") loaded.stringTopY = std::stof(value);
        else if (key == "stringBottomY") loaded.stringBottomY = std::stof(value);
        else if (key == "strumZoneLeft") loaded.strumZoneLeft = std::stof(value);
        else if (key == "strumZoneRight") loaded.strumZoneRight = std::stof(value);
        else if (key == "strumZoneTop") loaded.strumZoneTop = std::stof(value);
        else if (key == "strumZoneBottom") loaded.strumZoneBottom = std::stof(value);
        else if (key == "extendedThreshold") loaded.extendedThreshold = std::stof(value);
        else if (key == "referenceHandSize") loaded.referenceHandSize = std::stof(value);
        else if (key == "leftHandFretting") loaded.leftHandFretting = (value == "1");
        else if (key == "funMode") loaded.funMode = (value == "1");
        else if (key == "minStrumSpeed") loaded.minStrumSpeed = std::stof(value);
        else if (key == "maxStrumSpeed") loaded.maxStrumSpeed = std::stof(value);
        else if (key == "strumCooldownMs") loaded.strumCooldownMs = std::stoi(value);
        else if (key.size() > 12 && key.substr(0, 12) == "funModeChord")
        {
            int idx = std::stoi(key.substr(12));
            if (idx >= 0 && idx < 6)
                loaded.funModeChords[idx] = std::stoi(value);
        }
    }

    bool valid = true;
    if (loaded.strumZoneLeft >= loaded.strumZoneRight)
        valid = false;
    else if (loaded.strumZoneTop >= loaded.strumZoneBottom)
        valid = false;
    else if (loaded.strumZoneRight - loaded.strumZoneLeft < 0.05f)
        valid = false;
    else if (loaded.strumZoneBottom - loaded.strumZoneTop < 0.05f)
        valid = false;
    else if (loaded.stringTopY >= loaded.stringBottomY)
        valid = false;
    else if (loaded.stringBottomY - loaded.stringTopY < 0.05f)
        valid = false;
    else if (loaded.fret1X < 0.0f || loaded.fret1X > 1.0f)
        valid = false;
    else if (loaded.fret12X < 0.0f || loaded.fret12X > 1.0f)
        valid = false;

    if (!valid)
        return false;

    cal = loaded;
    return true;
}

} // namespace AirGuitar
