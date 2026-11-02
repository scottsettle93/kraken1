#pragma once

#include <istream>
#include <string>
#include <vector>

namespace sensor {

struct SensorReading {
    std::string type;
    double value;
};

std::vector<SensorReading> readReadings(std::istream& input);

}
