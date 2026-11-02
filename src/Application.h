#pragma once

#include <ostream>
#include <string>
#include <vector>

namespace sensor {

int runSensorMonitor(const std::vector<std::string>& arguments,
                     std::ostream& output, std::ostream& errors);

}
