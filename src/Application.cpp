#include "Application.h"
#include "SensorReader.h"
#include "Statistics.h"

#include <exception>
#include <fstream>
#include <iomanip>
#include <locale>
#include <stdexcept>

namespace sensor {

int runSensorMonitor(const std::vector<std::string>& arguments,
                     std::ostream& output, std::ostream& errors) {
    if (arguments.size() != 1) {
        errors << "Usage: sensor_monitor <file>\n";
        return 2;
    }
    const auto& filename = arguments.front();
    try {
        std::ifstream input(filename);
        if (!input) {
            throw std::runtime_error("cannot open input file");
        }
        const auto averages = calculateAverages(readReadings(input));
        output.imbue(std::locale::classic());
        output << std::defaultfloat << std::setprecision(10);
        for (const auto& [type, average] : averages) {
            output << type << " avg: " << average << '\n';
        }
        output.flush();
        if (!output) {
            throw std::runtime_error("failed to write output");
        }
        return 0;
    } catch (const std::exception& error) {
        errors << filename << ": " << error.what() << '\n';
        return 1;
    }
}

}
