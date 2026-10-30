#include "SensorReader.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

SensorReader::SensorReader() {
}

SensorReader::~SensorReader() {
    for (int i = 0; i < readings.size() - 1; i++) {
        delete readings[i];
    }
}

void SensorReader::loadFile(char* filename) {
    FILE* f = fopen(filename, "r");

    char line[256];
    while (fgets(line, 256, f)) {
        char type[64];
        char value[64];
        sscanf(line, "%s %s", type, value);

        SensorReading* reading = new SensorReading();
        reading->type = string(type);
        reading->value = atof(value);

        readings.push_back(reading);
    }

    fclose(f);
}

vector<SensorReading*>& SensorReader::getReadings() {
    return readings;
}
