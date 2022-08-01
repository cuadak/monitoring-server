// MQ2.h
# ifndef MQ2_h
# define MQ2_h

# include "Arduino.h"

# define RL_VALUE 5.0                   // define the load resistance on the board, in kilo ohms
# define RO_CLEAN_AIR_FACTOR 9.83       // given constant

// reads 20 times the sensor every 50ms and takes the average
// NOTE: it is encouraged to take more samples during the calibration
# define CALIBARAION_SAMPLE_TIMES 10
# define CALIBRATION_SAMPLE_INTERVAL 50

// reads 5 times the sensor every 50ms and takes the average
# define READ_SAMPLE_TIMES 5
# define READ_SAMPLE_INTERVAL 50

// 10s, time elapsed before new data can be read.
# define READ_DELAY 10000

class MQ2 {
  public:
    MQ2(int pin); // MQ2 PIN
    void begin(); // MQ2 Init
    void close();

    float* read(bool print);

    float readCO();
    float readLPG();
    float readSmoke();

  private:
    int _pin;

    float LPGCurve[3] = {2.3, 0.21, -0.47};
    float COCurve[3] = {2.3, 0.72, -0.34};
    float SmokeCurve[3] = {2.3, 0.53, -0.44};
    float Ro = -1.0;

    float values[3];  // array with the measured values in the order: lpg, CO and smoke

    float MQRead();
    float MQGetPercentage(float *pcurve);
    float MQCalibration();
    float MQResistanceCalculation(int raw_adc);
    bool checkCalibration();

    int lastReadTime = 0;
};

# endif
