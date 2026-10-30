cmake_minimum_required(VERSION 3.5)
project(sensor_monitor)

add_executable(sensor_monitor
    src/main.cpp
    src/SensorReader.cpp
    src/Statistics.cpp
)
