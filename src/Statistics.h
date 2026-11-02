#pragma once

#include "SensorReader.h"

#include <map>
#include <string>
#include <vector>

namespace sensor {

using SensorAverages = std::map<std::string, double>;

SensorAverages calculateAverages(const std::vector<SensorReading>& readings);

}
