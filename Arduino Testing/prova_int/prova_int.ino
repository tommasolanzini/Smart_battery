#include <Arduino.h>
#include "rf.h"
#include <Adafruit_INA219.h>

// Useful Substitutions
#define pinTemp1 A0
#define pinTemp2 A1
#define pinTemp3 A2
#define VREF 4.5

// ----------------- INA219 Setup -----------------
Adafruit_INA219 ina219;


// ------------- ARDUINO SETUP -------------
void setup() {
  Serial.begin(9600);
  Serial.println("ML test started");
  delay(50);

  // Initialize INA219
  if (!ina219.begin()) {
    Serial.println("Error: INA219 not found!");
    while (1) { delay(10); }
  }
  ina219.setCalibration_32V_1A();  // Default calibration (0.1 Ω)
  Serial.println("INA219 ready");
}

// ------------- FUNCTIONS -------------

void ReadTemperature(float* temp){
    int raw1 = analogRead(pinTemp1);
    temp[0] = raw1 * 450 / 1023 - 50;
    int raw2 = analogRead(pinTemp2);
    temp[1] = raw2 * 450 / 1023 - 50;
    int raw3 = analogRead(pinTemp3);
    temp[2] = raw3 * 450 / 1023 - 50;
    return;
};
float voting_stat(float m[3], bool flag[3]){
  float weight[3];
  float val;
  for (int i = 0; i < 3; i++) {
    if (isnan(m[i])) {
       weight[i] = -100;
    }
  }
  float d[6];
  d[0] = abs(m[1]-m[0]);
  d[1] = abs(m[1]-m[2]);
  d[2] = abs(m[2]-m[0]);
  float tot_sum = d[0]+d[1]+d[2];
  if (tot_sum == 0) {return m[0];}
  weight[0] = 1-(d[0]+d[2])/(tot_sum);
  weight[1] = 1-(d[0]+d[1])/(tot_sum);
  weight[2] = 1-(d[2]+d[1])/(tot_sum);
  
  float minweight = weight[0];
  int minIdx = 0;
  for (int i = 0; i < 3; i++) {
    if (weight[i] < minweight) {
      minweight = weight[i];
      minIdx = i;
    }
  }

  // potrebbe essere dispendioso, verificare
  if (minweight < 0.1) {
    float remaining_weight = 0;
    for (int i = 0; i < 3; i++) {
      if (i != minIdx) {
        val += m[i] * weight[i];
        remaining_weight += weight[i];
      }
    }
    val = val / remaining_weight;
    flag2 = 1;
    EEPROM.put(EEPROM_flag2, flag2);
    return val;
  } else {
  val = weight[0]*m[0] + weight[1]*m[1] + weight[2]*m[2];
  flag2 = 0;
  EEPROM.put(EEPROM_flag2, flag2);
  return val;
  }
};

// ------------- VARIABLES -------------
int16_t input[3];
float voltage;
float current;
float temp_vec[3];
bool flag[3];
int16_t temperature;

void loop() {

  // --- Read INA219 ---
  voltage = ina219.getBusVoltage_V();      
  current = ina219.getCurrent_mA(); 

  // --- Read TMP36 ---
  ReadTemperature(temp_vec);          // Temperature on load
  temperature = voting_stat(temp_vec, flag)

  // --- Create Input ---
  input[0] = voltage * 5e3;
  input[1] = temperature * 1e3;
  input[2] = current * (-10);

  // --- Predict ---
  int16_t prediction = RandomForest_predict(input, 3);

  // --- Print all results ---
  Serial.print("Bus Voltage: "); Serial.print(voltage); Serial.println(" V");
  Serial.print("Current: "); Serial.print(current); Serial.println(" mA");
  Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" °C");
  Serial.print("Prediction: "); Serial.print(prediction/327); Serial.println(" %");
  Serial.println("------------------------");


  delay(50);
}
