#pragma once

#include <map>
#include <vector>
#include "SensorReader.h"

class Statistics {
public:
    void calculate(vector<SensorReading*>& readings);
    void printResults();

private:
    map<string, double> averages;
};
