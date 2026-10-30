#include "Statistics.h"
#include <iostream>

void Statistics::calculate(vector<SensorReading*>& readings) {
    map<string, int> sums;
    map<string, int> counts;

    for (int i = 0; i < readings.size(); i++) {
        SensorReading* r = readings[i];
        sums[r->type] += r->value;
        counts[r->type]++;
    }

    for (auto it = counts.begin(); it != counts.end(); it++) {
        string type = it->first;
        int count = it->second;
        averages[type] = sums[type] / count;
    }
}

void Statistics::printResults() {
    for (auto it = averages.begin(); it != averages.end(); it++) {
        cout << it->first << " avg: " << it->second << endl;
    }
}
