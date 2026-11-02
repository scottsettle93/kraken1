#include "Application.h"

#include <iostream>

int main(int argc, char* argv[]) {
    return sensor::runSensorMonitor({argv + 1, argv + argc}, std::cout, std::cerr);
}
