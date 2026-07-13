#pragma once
#include "Physics/CalibrationData.h"
#include <string>

namespace AirGuitar {

class CalibrationManager
{
public:
    CalibrationManager();

    bool save(const CalibrationData& cal, const std::string& path);
    bool load(CalibrationData& cal, const std::string& path);

    std::string getDefaultPath() const;
    bool hasSavedCalibration() const;

private:
    std::string calibrationPath;
};

} // namespace AirGuitar
