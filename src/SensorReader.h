#include <vector>
#include <string>

using namespace std;

struct SensorReading {
    string type;
    double value;
};

class SensorReader {
public:
    SensorReader();
    ~SensorReader();

    void loadFile(char* filename);
    vector<SensorReading*>& getReadings();

private:
    vector<SensorReading*> readings;
};
