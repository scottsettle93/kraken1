#include "Statistics.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace sensor {

SensorAverages calculateAverages(const std::vector<SensorReading>& readings) {
    struct Total {
        double scale = 0;
        double sum = 0;
        double correction = 0;
        int exponent = 0;
        std::size_t count = 0;
    };
    std::map<std::string, Total> totals;
    for (const auto& reading : readings) {
        if (!std::isfinite(reading.value)) {
            throw std::invalid_argument("readings must be finite");
        }
        auto& total = totals[reading.type];
        total.scale = std::max(total.scale, std::abs(reading.value));
        ++total.count;
    }

    for (const auto& reading : readings) {
        auto& total = totals.at(reading.type);
        if (total.scale == 0) {
            continue;
        }
        total.exponent = std::ilogb(total.scale);
        const double value = std::scalbn(reading.value, -total.exponent);
        const double next = total.sum + value;
        const double added = next - total.sum;
        total.correction += (total.sum - (next - added)) + (value - added);
        total.sum = next;
    }

    SensorAverages averages;
    for (const auto& [type, total] : totals) {
        const double normalized = (total.sum + total.correction) / static_cast<double>(total.count);
        const double bound = std::scalbn(total.scale, -total.exponent);
        averages.emplace(type, std::scalbn(std::clamp(normalized, -bound, bound), total.exponent));
    }
    return averages;
}

}
