#include <iostream>
#include "Statistics.h"

using namespace std;

int main(int argc, char* argv[]) {
    char* filename = argv[1];

    SensorReader reader;
    reader.loadFile(filename);

    Statistics stats;
    stats.calculate(reader.getReadings());
    stats.printResults();

    return 0;
}
