# Sensor Monitoring

Sensor readings are grouped by sensor type to calculate an average for each type.

## Language

**Sensor reading**:
A finite numeric measurement associated with a sensor type.
_Avoid_: Sensor, when referring to an individual measurement.

**Sensor type**:
The category of measurement, such as `TEMP`, `PRESSURE`, or `HUMIDITY`.
Names are case-sensitive; differently capitalized names denote distinct types.
_Avoid_: Sensor ID, which would identify a particular physical device.

**Average**:
The arithmetic mean of the readings belonging to one sensor type: their sum
divided by their count.
_Avoid_: Median, total.
