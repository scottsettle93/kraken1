# Per-function line-based CRAP

CRAP = CC² × (1 − per-function line coverage)³ + CC.
This uses line coverage, not the original basis-path coverage definition.
Complexity is Lizard's source-level estimate. No missing measurement is assumed covered.

| Function | Source | CC | Lines | Branches | CRAP (line) |
| --- | --- | ---: | ---: | ---: | ---: |
| sensor::runSensorMonitor | src/Application.cpp:13 | 6 | 21/21 | 29/29 | 6.000 |
| sensor::parseValue | src/SensorReader.cpp:13 | 6 | 11/11 | 18/18 | 6.000 |
| sensor::readReadings | src/SensorReader.cpp:30 | 7 | 23/23 | 35/35 | 7.000 |
| sensor::calculateAverages | src/Statistics.cpp:10 | 6 | 25/25 | 15/15 | 6.000 |
| main | src/main.cpp:5 | 1 | 2/2 | 2/2 | 1.000 |

The historical warning threshold of 30 is a heuristic, not a correctness guarantee.
