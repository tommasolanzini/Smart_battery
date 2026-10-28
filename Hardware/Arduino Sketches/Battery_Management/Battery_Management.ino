#include <Wire.h>
#include <Adafruit_INA219.h>
#include <math.h>  // Needed for sqrt()

// ----------------- INA219 Setup -----------------
Adafruit_INA219 ina219;

// ----------------- TMP36 Setup -----------------
#define TempSensor1 A0  // First temperature sensor
#define TempSensor2 A1  // Second temperature sensor
#define N 100           // Number of measurements per cycle
#define VREF 3.3      // Internal reference voltage (Arduino INTERNAL ~1.1V)

// ----------------- Setup -----------------
void setup() {
  Serial.begin(9600);
  while (!Serial) { delay(10); }

  // Initialize INA219
  if (!ina219.begin()) {
    Serial.println("Error: INA219 not found!");
    while (1) { delay(10); }
  }
  ina219.setCalibration_32V_2A();  // Default calibration (0.1 Ω)
  Serial.println("INA219 ready");

  // Use internal voltage reference for TMP36 readings
  analogReference(AR_INTERNAL);
}

// ----------------- TMP36 Functions -----------------
// Read a sensor N times, convert voltage to °C
void readSensor(float values[], int pin) {
  for (int i = 0; i < N; i++) {
    float voltage = analogRead(pin) * (VREF / 4095.0);
    values[i] = (voltage - 0.5) * 100.0;  // TMP36 formula
    delay(50);
  }
}

// Calculate mean of an array
float calculateMean(float values[]) {
  float sum = 0;
  for (int i = 0; i < N; i++) sum += values[i];
  return sum / N;
}

// Calculate standard deviation of an array
float calculateStdDev(float values[], float mean) {
  float sum = 0;
  for (int i = 0; i < N; i++) {
    float diff = values[i] - mean;
    sum += diff * diff;
  }
  return sqrt(sum / N);
}

// ----------------- Main Loop -----------------
void loop() {
  // --- Read INA219 ---
  float busVoltage = ina219.getBusVoltage_V();      // Voltage on load
  float shuntVoltage = ina219.getShuntVoltage_mV(); // Voltage across shunt
  float current_mA = ina219.getCurrent_mA() * 2.0;  // Correct for 0.2 Ω shunt
  float power_mW = busVoltage * current_mA;         // Power in mW

  // --- Read TMP36 sensors ---
  float values1[N], values2[N];
  readSensor(values1, TempSensor1);
  readSensor(values2, TempSensor2);

  float mean1 = calculateMean(values1);
  float mean2 = calculateMean(values2);

  float stdDev1 = calculateStdDev(values1, mean1);
  float stdDev2 = calculateStdDev(values2, mean2);

  // Weight sensors inversely proportional to std deviation
  float weight1 = stdDev2 / (stdDev1 + stdDev2);
  float weight2 = stdDev1 / (stdDev1 + stdDev2);

  float finalTemperature = mean1 * weight1 + mean2 * weight2;

  // --- Print all results ---
  Serial.print("Bus Voltage: "); Serial.print(busVoltage); Serial.println(" V");
  Serial.print("Current: "); Serial.print(current_mA); Serial.println(" mA");
  Serial.print("Power: "); Serial.print(power_mW); Serial.println(" mW");

  Serial.print("Temperature: "); Serial.print(finalTemperature); Serial.println(" °C");
  Serial.println("------------------------");

  delay(1000);
}
