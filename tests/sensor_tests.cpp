#include "Application.h"
#include "SensorReader.h"
#include "Statistics.h"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <locale>
#include <random>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <vector>

namespace {

sensor::SensorAverages averagesFor(const std::string& text) {
    std::istringstream input(text);
    return sensor::calculateAverages(sensor::readReadings(input));
}

class TemporaryInput {
public:
    explicit TemporaryInput(const std::string& contents) {
        std::random_device random;
        for (int attempt = 0; attempt < 16; ++attempt) {
            const auto candidate = std::filesystem::temp_directory_path() /
                ("sensor-monitor-test-" + std::to_string(random()) + "-" + std::to_string(random()));
            if (std::filesystem::create_directory(candidate)) {
                directory = candidate;
                break;
            }
        }
        if (directory.empty()) {
            throw std::runtime_error("cannot create test directory");
        }
        path = (directory / "readings.txt").string();
        std::ofstream file(path);
        file << contents;
        if (!file) {
            throw std::runtime_error("cannot write test input");
        }
    }

    ~TemporaryInput() {
        std::error_code error;
        std::filesystem::remove_all(directory, error);
    }

    TemporaryInput(const TemporaryInput&) = delete;
    TemporaryInput& operator=(const TemporaryInput&) = delete;

    std::string path;

private:
    std::filesystem::path directory;
};

TEST(Readings, CalculatesInstructionExample) {
    const auto result = averagesFor(
        "TEMP 23.4\nTEMP 22.8\nPRESSURE 101.2\nTEMP 21.9\nPRESSURE 100.9\n");
    ASSERT_EQ(result.size(), 2U);
    EXPECT_DOUBLE_EQ(result.at("TEMP"), 22.7);
    EXPECT_DOUBLE_EQ(result.at("PRESSURE"), 101.05);
}

TEST(Readings, PreservesFractionsDuringAccumulationAndDivision) {
    const auto result = averagesFor("A 0.9\nA 0.9\nB 1\nB 2\n");
    EXPECT_DOUBLE_EQ(result.at("A"), 0.9);
    EXPECT_DOUBLE_EQ(result.at("B"), 1.5);
}

TEST(Readings, AcceptsBlankLinesTabsCrLfAndMissingFinalNewline) {
    const auto result = averagesFor("\n \t\r\nTEMP\t2\r\n\nTEMP 4");
    ASSERT_EQ(result.size(), 1U);
    EXPECT_DOUBLE_EQ(result.at("TEMP"), 3);
}

TEST(Readings, EmptyAndWhitespaceOnlyInputsHaveNoAverages) {
    EXPECT_TRUE(averagesFor("").empty());
    EXPECT_TRUE(averagesFor(" \t\r\n\n").empty());
}

TEST(Readings, AcceptsSignedDecimalsAndScientificNotation) {
    const auto result = averagesFor("A +1.5e2\nA -5e1\nB .25\nB 1.\nC -0\n");
    EXPECT_DOUBLE_EQ(result.at("A"), 50);
    EXPECT_DOUBLE_EQ(result.at("B"), 0.625);
    EXPECT_DOUBLE_EQ(result.at("C"), 0);
}

TEST(Readings, DistinguishesArbitraryCaseSensitiveSensorTypes) {
    const auto result = averagesFor("custom/sensor 7\nTEMP 1\ntemp 9\n");
    ASSERT_EQ(result.size(), 3U);
    EXPECT_DOUBLE_EQ(result.at("custom/sensor"), 7);
    EXPECT_DOUBLE_EQ(result.at("TEMP"), 1);
    EXPECT_DOUBLE_EQ(result.at("temp"), 9);
}

TEST(Readings, HandlesLongLabelsAndLinesWithoutSplittingRecords) {
    const std::string label(1024, 'x');
    const auto result = averagesFor(label + std::string(300, ' ') + "2\n" + label + " 4\n");
    ASSERT_EQ(result.size(), 1U);
    EXPECT_DOUBLE_EQ(result.at(label), 3);
    EXPECT_DOUBLE_EQ(averagesFor("A " + std::string(100, '0') + "1\n").at("A"), 1);
}

class InvalidRecord : public testing::TestWithParam<const char*> {};

TEST_P(InvalidRecord, RejectsMalformedDataAtPhysicalLineNumber) {
    std::istringstream input(std::string("OK 1\n\n") + GetParam() + "\n");
    try {
        sensor::readReadings(input);
        FAIL() << "accepted invalid record: " << GetParam();
    } catch (const std::runtime_error& error) {
        EXPECT_NE(std::string(error.what()).find("line 3:"), std::string::npos);
    }
}

INSTANTIATE_TEST_SUITE_P(Validation, InvalidRecord, testing::Values(
    "TEMP", "TEMP nope", "TEMP 12junk", "TEMP 2 ignored", "TEMP nan",
    "TEMP NaN", "TEMP inf", "TEMP -inf", "TEMP +inf", "TEMP 1e9999",
    "TEMP 1e-9999", "TEMP +", "TEMP --1", "TEMP ++1", "TEMP +-1",
    "TEMP 0x1p2", "TEMP 1,5", "# comment"));

TEST(Readings, ReportsBadOrPrematurelyFailedStreams) {
    std::istringstream bad("TEMP 1\n");
    bad.setstate(std::ios::badbit);
    EXPECT_THROW(sensor::readReadings(bad), std::runtime_error);
    std::istringstream failed("TEMP 1\n");
    failed.setstate(std::ios::failbit);
    EXPECT_THROW(sensor::readReadings(failed), std::runtime_error);
}

TEST(Statistics, EachCallReturnsIndependentResults) {
    const auto first = sensor::calculateAverages({{"A", 2}});
    const auto second = sensor::calculateAverages({{"B", 8}});
    ASSERT_EQ(first.size(), 1U);
    ASSERT_EQ(second.size(), 1U);
    EXPECT_EQ(first.count("B"), 0U);
    EXPECT_EQ(second.count("A"), 0U);
    EXPECT_TRUE(sensor::calculateAverages({}).empty());
}

TEST(Statistics, SupportsValuesBeyondIntegerRangeAndLargeFiniteTotals) {
    const double maximum = std::numeric_limits<double>::max();
    const auto result = sensor::calculateAverages(
        {{"A", 2147483648.0}, {"A", 2147483650.0},
         {"B", maximum}, {"B", maximum}, {"C", -maximum}, {"C", maximum}});
    EXPECT_DOUBLE_EQ(result.at("A"), 2147483649.0);
    EXPECT_DOUBLE_EQ(result.at("B"), maximum);
    EXPECT_DOUBLE_EQ(result.at("C"), 0);
}

TEST(Statistics, CompensatedAccumulationPreservesSmallTermsInCancellation) {
    const auto result = sensor::calculateAverages({{"A", 1e16}, {"A", 1}, {"A", -1e16}});
    EXPECT_DOUBLE_EQ(result.at("A"), 1.0 / 3.0);
}

TEST(Statistics, PreservesRepresentableDifferencesDuringCancellation) {
    const auto result = sensor::calculateAverages(
        {{"A", 1e16}, {"A", 9999999999999998.0}, {"A", -1e16}, {"A", -1e16}});
    EXPECT_DOUBLE_EQ(result.at("A"), -0.5);
}

TEST(Statistics, AveragesLargeBatchesOfExtremeFiniteValues) {
    const double maximum = std::numeric_limits<double>::max();
    std::vector<sensor::SensorReading> readings;
    for (int index = 0; index < 4096; ++index) {
        readings.push_back({"POSITIVE", maximum});
        readings.push_back({"NEGATIVE", -maximum});
        readings.push_back({"CANCELLATION", maximum});
    }
    for (int index = 0; index < 4096; ++index) {
        readings.push_back({"CANCELLATION", -maximum});
    }
    const auto result = sensor::calculateAverages(readings);
    ASSERT_EQ(result.size(), 3U);
    EXPECT_TRUE(std::isfinite(result.at("POSITIVE")));
    EXPECT_TRUE(std::isfinite(result.at("NEGATIVE")));
    EXPECT_TRUE(std::isfinite(result.at("CANCELLATION")));
    EXPECT_DOUBLE_EQ(result.at("POSITIVE"), maximum);
    EXPECT_DOUBLE_EQ(result.at("NEGATIVE"), -maximum);
    EXPECT_DOUBLE_EQ(result.at("CANCELLATION"), 0);
}

TEST(Statistics, PreservesSubnormalAndZeroAverages) {
    const double smallest = std::numeric_limits<double>::denorm_min();
    const auto result = sensor::calculateAverages(
        {{"TINY", smallest}, {"TINY", smallest}, {"ZERO", 0}, {"ZERO", -0.0}});
    EXPECT_EQ(result.at("TINY"), smallest);
    EXPECT_EQ(result.at("ZERO"), 0);
}

TEST(Statistics, RejectsNonfiniteValuesFromDirectCallers) {
    EXPECT_THROW(sensor::calculateAverages({{"A", std::numeric_limits<double>::infinity()}}),
                 std::invalid_argument);
    EXPECT_THROW(sensor::calculateAverages({{"A", std::numeric_limits<double>::quiet_NaN()}}),
                 std::invalid_argument);
}

TEST(Application, PrintsSortedAveragesWithTenSignificantDigits) {
    TemporaryInput input("Z 1\nZ 2\nZ 2\nA 4\n");
    std::ostringstream output;
    std::ostringstream errors;
    EXPECT_EQ(sensor::runSensorMonitor({input.path}, output, errors), 0);
    EXPECT_EQ(output.str(), "A avg: 4\nZ avg: 1.666666667\n");
    EXPECT_TRUE(errors.str().empty());
}

TEST(Application, EmptyInputSucceedsWithoutOutput) {
    TemporaryInput input("");
    std::ostringstream output;
    std::ostringstream errors;
    EXPECT_EQ(sensor::runSensorMonitor({input.path}, output, errors), 0);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(errors.str().empty());
}

TEST(Application, InvalidInputReportsFileAndLineWithoutPartialResults) {
    TemporaryInput input("A 5\nB bad\n");
    std::ostringstream output;
    std::ostringstream errors;
    EXPECT_EQ(sensor::runSensorMonitor({input.path}, output, errors), 1);
    EXPECT_TRUE(output.str().empty());
    EXPECT_NE(errors.str().find(input.path + ": line 2:"), std::string::npos);
}

TEST(Application, RequiresExactlyOneArgument) {
    std::ostringstream output;
    std::ostringstream errors;
    EXPECT_EQ(sensor::runSensorMonitor({}, output, errors), 2);
    EXPECT_EQ(sensor::runSensorMonitor({"one", "two"}, output, errors), 2);
    EXPECT_TRUE(output.str().empty());
    EXPECT_NE(errors.str().find("Usage:"), std::string::npos);
}

TEST(Application, ReportsMissingFile) {
    TemporaryInput input("");
    std::remove(input.path.c_str());
    std::ostringstream output;
    std::ostringstream errors;
    EXPECT_EQ(sensor::runSensorMonitor({input.path}, output, errors), 1);
    EXPECT_TRUE(output.str().empty());
    EXPECT_NE(errors.str().find("cannot open input file"), std::string::npos);
}

TEST(Application, ReportsOutputFailure) {
    TemporaryInput input("A 2\n");
    std::ostringstream output;
    output.setstate(std::ios::badbit);
    std::ostringstream errors;
    EXPECT_EQ(sensor::runSensorMonitor({input.path}, output, errors), 1);
    EXPECT_NE(errors.str().find("failed to write output"), std::string::npos);
}

class ThrowingOutputBuffer : public std::streambuf {
public:
    struct Failure {};

protected:
    int_type overflow(int_type) override {
        throw Failure{};
    }
};

TEST(Application, PropagatesUnexpectedOutputExceptions) {
    TemporaryInput input("A 2\n");
    ThrowingOutputBuffer buffer;
    std::ostream output(&buffer);
    output.exceptions(std::ios::badbit);
    std::ostringstream errors;
    EXPECT_THROW(sensor::runSensorMonitor({input.path}, output, errors),
                 ThrowingOutputBuffer::Failure);
    EXPECT_TRUE(output.bad());
    EXPECT_TRUE(errors.str().empty());
}

class CommaDecimal : public std::numpunct<char> {
protected:
    char do_decimal_point() const override {
        return ',';
    }
};

TEST(Application, OutputUsesStableFormattingIndependentOfStreamSettings) {
    TemporaryInput input("A 1.5\n");
    std::ostringstream output;
    output.imbue(std::locale(std::locale::classic(), new CommaDecimal));
    output.setf(std::ios::fixed, std::ios::floatfield);
    std::ostringstream errors;
    EXPECT_EQ(sensor::runSensorMonitor({input.path}, output, errors), 0);
    EXPECT_EQ(output.str(), "A avg: 1.5\n");
}

}
