#include <Wire.h>
#include <Adafruit_INA219.h>

#define CUTOFF_VOLTAGE 3.2    
#define SAMPLE_TIME   1000    

Adafruit_INA219 ina219;

unsigned long t0;
unsigned long lastSample;

float capacity_mAh = 0;

void setup() {

  Serial.begin(115200);
  while (!Serial);

  Wire.begin();

  if (!ina219.begin()) {
    Serial.println("ERRORE: INA219 non trovato!");
    while (1);
  }

  // Calibrazione standard (fino a ~3A)
  ina219.setCalibration_32V_2A();

  delay(1000);

  // Header CSV
  Serial.println("Time_s,Voltage_V,Current_A,Capacity_mAh");

  t0 = millis();
  lastSample = millis();
}

void loop() {

  if (millis() - lastSample >= SAMPLE_TIME) {

    lastSample = millis();

// Readings
    float voltage = ina219.getShuntVoltage_mV() / 1000.0;
    float current = ina219.getCurrent_mA() / 1000.0; 
    float time_s = (millis() - t0) / 1000.0;

    float dt_h = SAMPLE_TIME / 3600000.0;
    capacity_mAh += current * dt_h * 1000.0;

    // =====  CSV =====
    Serial.print(time_s, 1);
    Serial.print(",");
    Serial.print(voltage, 3);
    Serial.print(",");
    Serial.print(current, 3);
    Serial.print(",");
    Serial.println(capacity_mAh, 2);

    //  Exit Condition
    if (voltage < CUTOFF_VOLTAGE) {
      Serial.println("END,0,0,0");
      while (1); 
    }
  }
}
