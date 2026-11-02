#include "SensorReader.h"

#include <charconv>
#include <cmath>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace sensor {
namespace {

double parseValue(const std::string& token, std::size_t lineNumber) {
    const char* begin = token.data();
    const char* end = begin + token.size();
    if (token.front() == '+' && token[1] != '-') {
        ++begin;
    }
    double value = 0;
    const auto result = std::from_chars(begin, end, value, std::chars_format::general);
    if (result.ec != std::errc{} || result.ptr != end || !std::isfinite(value)) {
        throw std::runtime_error("line " + std::to_string(lineNumber) +
                                 ": expected a finite decimal value");
    }
    return value;
}

}

std::vector<SensorReading> readReadings(std::istream& input) {
    std::vector<SensorReading> readings;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        std::istringstream fields(line);
        fields.imbue(std::locale::classic());
        std::string type;
        if (!(fields >> type)) {
            continue;
        }
        std::string token;
        std::string extra;
        if (!(fields >> token) || (fields >> extra)) {
            throw std::runtime_error("line " + std::to_string(lineNumber) +
                                     ": expected SENSOR_TYPE VALUE");
        }
        const double value = parseValue(token, lineNumber);
        readings.push_back({std::move(type), value});
    }
    if (input.bad() || !input.eof()) {
        throw std::runtime_error("failed to read input");
    }
    return readings;
}

}
