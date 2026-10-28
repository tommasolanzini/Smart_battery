#include <Arduino.h>
#include <Adafruit_INA219.h>
#include <avr/wdt.h>      // Sostituisce WDT.h per Arduino UNO (AVR)
#include <EEPROM.h>
#include <Wire.h>
#include <SoftwareSerial.h> // Necessario per avere una seconda seriale su UNO

// DEFINIZIONE PIN SERIAL1 (SoftwareSerial)
// Collega il dispositivo esterno a questi pin:
// Pin 10 Arduino -> TX del dispositivo
// Pin 11 Arduino -> RX del dispositivo
SoftwareSerial Serial1(10, 11); 

// INCLUSIONE FILE ESTERNO
// Assicurati che questo file sia compatibile con la memoria limitata di UNO (2KB RAM)
#include "rf.h" 

#define pinTemp1 A0
#define pinTemp2 A2
#define pinTemp3 A3
#define VREF_TEMP 3.5
#define IntResistance 0.5

#define MinTemp -10
#define MaxTemp 70
#define MaxGradTemp 5
#define MaxDistTemp 5
#define MaxCurr 2
#define MinCurr -0.1
#define MaxGradCurr 0.5
#define MaxDistCurr 0.3
#define MaxVolt 4.5
#define MinVolt 1
#define MaxGradVolt 0.5
#define MaxDistVolt 0.3
#define MaxPen 30

// ----------------- INA219 Setup -----------------

Adafruit_INA219 ina219_A(0x44);
Adafruit_INA219 ina219_B(0x41);

// ----------------- VARIABLES -----------------

// Voltage Variables
float voltage;
float volt_vec[2];
int faulty_volt[2] = { 0 };
float OldVolt[2] = { 0 };

// Current Variables
float current;
float curr_vec[2];
int faulty_curr[2] = { 0 };
float OldCurr[2] = { 0 };

// Temperature Variables
float temperature;
float temp_vec[3];
int faulty_temp[3] = { 0 };
float OldTemp[3] = { 0 };

// Prediction Variables
float prediction;
int16_t input[3];
float OldPrediction = 0;

// Fault-Tolerance Variables
int idx1T = 0;
int idx1C = 0;
int idx1V = 0;
int idx2T = 0;
int idx2C = 0;
int idx2V = 0;
int modeT = 0;
int modeC = 0;
int modeV = 0;
const int MAX_WDT_ATTEMPTS = 3;
const int EEPROM_WDT_COUNT_ADDR = 20;
const int EEPROM_flag2 = 19;
const int RELAY_PIN = 7;

String msg;
unsigned long lastLoopTime = 0;
const unsigned long SAFETY_TIMEOUT = 2000000;

// ------------- SAFE MODE FUNCTIONS -------------

void EnterSafeMode(int idx1, int idx2) {  // FUNZIONE DI EMERGENZA (POST-CRASH)
  Serial.println(F("ENTERING SAFE MODE (System Halted)-")); // F() macro salva RAM
  Serial1.println(F("ENTERING SAFE MODE (System Halted)-"));
  
  if ((!idx1) && (!idx2)) {
    Serial.println(F("Cause of FAILURE: Previous Watchdog Timeout detected."));
    Serial1.println(F("Cause of FAILURE: Previous Watchdog Timeout detected."));
  } else if (!idx2) {
    Serial.print(F("Cause of FAILURE: Faulty current sensor, number: "));
    Serial.println(idx1);
    Serial1.print(F("Cause of FAILURE: Faulty current sensor, number: "));
    Serial1.println(idx1);
  } else if ((idx1) && (idx2)) {
    Serial.print(F("Cause of FAILURE: Faulty temperature sensors: "));
    Serial.print(idx1);
    Serial.print(F(" & "));
    Serial.println(idx2);
    Serial1.print(F("Cause of FAILURE: Faulty temperature sensors: "));
    Serial1.print(idx1);
    Serial1.print(F(" & "));
    Serial1.println(idx2);
  } else if (idx2 == 4) {
    Serial.print(F("Cause of FAILURE: Critical TEMPERATURE, cutting off power"));
    Serial1.print(F("Cause of FAILURE: Critical TEMPERATURE, cutting off power"));
    digitalWrite(RELAY_PIN, LOW);
  } else if (idx2 == 5) {
    Serial.print(F("Cause of FAILURE: Critical CURRENT, cutting off power"));
    Serial1.print(F("Cause of FAILURE: Critical CURRENT, cutting off power"));
    digitalWrite(RELAY_PIN, LOW);
  } else if (idx2 == 6) {
    Serial.print(F("Cause of FAILURE: Critical VOLTAGE, cutting off power"));
    Serial1.print(F("Cause of FAILURE: Critical VOLTAGE, cutting off power"));
    digitalWrite(RELAY_PIN, LOW);
  }
  
  while (1) {
    delay(50);
    wdt_reset(); // Reset del Watchdog per evitare riavvii infiniti in Safe Mode
  }
}

void checkResetReason() {  // LEGGE LA CAUSA DEL RESET (Adattato per AVR/Uno)
  uint8_t mcusr = MCUSR;
  MCUSR = 0; // Pulisce i flag subito
  
  byte wdtCount = 0;
  EEPROM.get(EEPROM_WDT_COUNT_ADDR, wdtCount);

  // Controlla il bit WDRF (Watchdog Reset Flag)
  if (mcusr & (1 << WDRF)) {
    Serial.println(F("!!! ALERT: SYSTEM REBOOTED BY WATCHDOG !!!"));
    
    wdtCount++;                              // Incrementa il contatore
    EEPROM.put(EEPROM_WDT_COUNT_ADDR, wdtCount);
    Serial.print(F("!!! ALERT: WATCHDOG RESET #"));
    Serial.println(wdtCount);

    if (wdtCount >= MAX_WDT_ATTEMPTS) {
      EnterSafeMode(0, 0);
    } else {
      Serial.println(F("Attempting normal reboot..."));
    }
  } else {
    // SE L'AVVIO È NORMALE (Power-on o Reset manuale)
    Serial.println(F("System boot: Normal. Resetting crash counter."));
    wdtCount = 0;
    EEPROM.put(EEPROM_WDT_COUNT_ADDR, wdtCount);  // Azzera il contatore
  }
}

// ------------- ARDUINO SETUP -------------

void setup() {
  Serial.begin(9600);
  while (!Serial) { delay(10); }
  
  Serial1.begin(9600); // Avvia SoftwareSerial sui pin 10, 11
  // while (!Serial1) { delay(10); } // Non necessario per SoftwareSerial

  checkResetReason();  // Controlla perché ci siamo accesi

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  Serial.println(F("ML test started"));
  Serial1.println(F("ML test started"));
  delay(50);

  if (!ina219_A.begin()) {
    Serial.println(F("INA219 A not found"));
    Serial1.println(F("INA219 A not found"));
    while (1);
  }
  ina219_A.setCalibration_32V_1A();
  Serial.println(F("INA219 A ready"));
  Serial1.println(F("INA219 A ready"));
  delay(50);

  if (!ina219_B.begin()) {
    Serial.println(F("INA219 B not found"));
    Serial1.println(F("INA219 B not found"));
    while (1);
  }
  ina219_B.setCalibration_32V_1A();
  Serial.println(F("INA219 B ready"));
  Serial1.println(F("INA219 B ready"));
  delay(50);

  // Setup Watchdog per AVR (2 secondi)
  wdt_enable(WDTO_2S);
  
  lastLoopTime = micros();  // Inizializza il timer
}

// ------------- FUNCTIONS -------------

void ReadTemperature(float* temp) {
  int raw1 = analogRead(pinTemp1);
  temp[0] = raw1 * VREF_TEMP * 100.0 / 1023.0 - 50.0;
  int raw2 = analogRead(pinTemp2);
  temp[1] = raw2 * VREF_TEMP * 100.0 / 1023.0 - 50.0;
  int raw3 = analogRead(pinTemp3);
  temp[2] = raw3 * VREF_TEMP * 100.0 / 1023.0 - 50.0;
  return;
};

void ReadCurrent(float* curr) {
  curr[0] = ina219_A.getCurrent_mA() / 1000.0;
  curr[1] = ina219_B.getCurrent_mA() / 1000.0;
}

void ReadVoltage(float* volt) {
  volt[0] = ina219_A.getBusVoltage_V() + 1.107 * ina219_A.getCurrent_mA() / 1000.0;
  volt[1] = ina219_B.getBusVoltage_V() + 0.84 * ina219_B.getCurrent_mA() / 1000.0;
}

float average(float x, float x_avg) {
  return (x_avg * 10 + x) / 11;
};

float Voting(float* vec, int* faulty, int len, float* old_vec, int& idx, int& mode, float dist, float max, float min, float grad) {
  // SANITY
  bool fault;
  for (int i = 0; i < len; i++) {
    fault =
      (vec[i] > max) || (vec[i] < min) || (fabs(vec[i] - old_vec[i]) > grad);

    if (fault && faulty[i] < MaxPen) {
      faulty[i] += 2;
    } else if (faulty[i] > 0) {
      faulty[i] -= 1;
    }
    old_vec[i] = vec[i];
  }

  // VOTING
  float val = 0;
  switch (len) {
    case 3:
      {
        float weight[3];
        float d[3];
        d[0] = fabs(vec[1] - vec[2]);
        d[1] = fabs(vec[2] - vec[0]);
        d[2] = fabs(vec[0] - vec[1]);
        float tot_sum = d[0] + d[1] + d[2];
        for (int i = 0; i < 3; i++) {
          weight[i] = d[i] / (tot_sum);
        }

        float minweight = weight[0];
        int minIdx = 0;

        for (int i = 0; i < 3; i++) {
          if (weight[i] < minweight) {
            minweight = weight[i];
            minIdx = i;
          }
        }

        if (tot_sum < dist) {
          val = weight[0] * vec[0] + weight[1] * vec[1] + weight[2] * vec[2];
          for (int i = 0; i < 3; i++) {
            if (faulty[i] > 0) {
              faulty[i] -= 1;
            }
          }
        } else if (minweight < 0.15) {
          float remaining_weight = 0;
          for (int i = 0; i < 3; i++) {
            if (i != minIdx) {
              val += vec[i] * weight[i];
              remaining_weight += weight[i];
            }
          }
          val = val / remaining_weight;
          for (int i = 0; i < 3; i++) {
            if (i == minIdx) {
              if ((faulty[minIdx] < MaxPen) && (d[minIdx] > dist)) {
                faulty[minIdx] += 2;
              }
            } else if (faulty[i] > 0) {
              faulty[i] -= 1;
            }
          }
        } else {
          val = weight[0] * vec[0] + weight[1] * vec[1] + weight[2] * vec[2];
          for (int i = 0; i < 3; i++) {
            if (faulty[i] < MaxPen) {
              faulty[i] += 2;
            }
          }
        }

        if (mode) {
          int j = 0;
          val = 0;
          for (int i = 0; i < len; i++) {
            if (i == idx - 1) {
              continue;
            }
            val += vec[i] * 0.5;
            j++;
          }
        }
        break;
      }
    case 2:
      {
        if (mode && idx >= 1 && idx <= 2) {
          val = vec[2 - idx];
        } else {
          val = (vec[0] + vec[1]) / 2;
        }
        break;
      }
  }
  return val;
}

void FaultHandling(int* faulty, int len, int& idx1, int& idx2, String type, int& mode) {

  if ((mode) && (faulty[idx1 - 1])) {
    for (int k = 0; k < len; k++) {
      if (k == idx1 - 1) {
        continue;
      }
      if (faulty[k] > MaxPen - 1) {
        idx2 = k + 1;
        EnterSafeMode(idx1, idx2);
        break;
      }
    }
    mode = 1;
    Serial.print(type);
    Serial.print(F(" - Faulty Sensor: #"));
    Serial.println(idx1);
    msg = String("Degraded Mode") + ",";
    return;
  } else if ((mode) && (!faulty[idx1 - 1])) {
    mode = 0;
  }

  idx1 = 0;
  idx2 = 0;

  if (len == 3)
    for (int i = 0; i < len; i++) {
      if (faulty[i] > MaxPen - 1) {
        idx1 = i + 1;
        for (int k = i + 1; k < len; k++) {
          if (faulty[k] > MaxPen - 1) {
            idx2 = k + 1;
            EnterSafeMode(idx1, idx2);
            break;
          }
        }
        break;
      }
    }
  else if (len == 2)
    for (int i = 0; i < len; i++) {
      if (faulty[i] > MaxPen - 1) {
        idx1 = i + 1;
        EnterSafeMode(idx1, idx2);
      }
    }

  if (idx1) {
    Serial.print(type);
    Serial.print(F(" - Faulty Sensor #"));
    Serial.println(idx1);
    msg = String("Degraded Mode") + ",";
    mode = 1;
  }
}

void SafetyMechanism(float temp, float curr, float volt) {
  int idx;
  if ((temp > 45) || (temp < -10)) {
    idx = 4;
    EnterSafeMode(0, idx);
  }
  if (curr > 2) {
    idx = 5;
    EnterSafeMode(0, idx);
  }
  if ((curr > 0.01) && (temp < 0)) {
    Serial.println(F("Temperature under 0°C while discharging, available capacity reduced"));
  } else if ((curr < -0.01) && (temp < 0)) {
    Serial.println(F("Temperature under 0°C while charging, damage"));
    idx = 4;
    EnterSafeMode(0, idx);
  } else if (volt < 3) {
    Serial.println(F("voltage under 3 V"));
    idx = 6;
    EnterSafeMode(0, idx);
  }
}

void loop() {
  // --- WD feeding ---
  wdt_reset(); // RESET DEL WATCHDOG AVR
  lastLoopTime = micros();

  // --- Readings ---
  ReadVoltage(volt_vec);  
  ReadCurrent(curr_vec);
  ReadTemperature(temp_vec);

  // --- Corrections ---
  for (int i = 0; i < 3; i++) {
    temp_vec[i] = average(temp_vec[i], OldTemp[i]);
  }

  // --- Voting & Fault Handling ---
  temperature = Voting(temp_vec, faulty_temp, 3, OldTemp, idx1T, modeT, MaxDistTemp, MaxTemp, MinTemp, MaxGradTemp);
  FaultHandling(faulty_temp, 3, idx1T, idx2T, "Temperature", modeT);

  current = Voting(curr_vec, faulty_curr, 2, OldCurr, idx1C, modeC, MaxDistCurr, MaxCurr, MinCurr, MaxGradCurr);
  FaultHandling(faulty_curr, 2, idx1C, idx2C, "Current", modeC);

  voltage = Voting(volt_vec, faulty_volt, 2, OldVolt, idx1V, modeV, MaxDistVolt, MaxVolt, MinVolt, MaxGradVolt);
  FaultHandling(faulty_volt, 2, idx1V, idx2V, "Voltage", modeV);
  
  // --- Internal Resistance
  voltage += current * IntResistance;

  // --- Create Input ---
  input[0] = voltage * 5e3;
  input[1] = temperature * 1e3;
  input[2] = current * (-10);

  // --- Predict ---
  // ATTENZIONE: Assicurati che rf_big.h e la funzione RandomForest siano ottimizzati per UNO (poca RAM)
  prediction = RandomForest_predict(input, 3) / 327;
  prediction = average(prediction, OldPrediction);
  OldPrediction = prediction;

  if ((modeT) ^ (modeC) ^ (modeV)) {
    Serial.println(F("Degraded Mode"));
  } else {
    Serial.println(F("Nominal Mode"));
    msg = String("Nominal Mode") + ",";
  }

  Serial.print(F("Temperature: "));
  Serial.print(temperature);
  Serial.println(F(" °C"));
  Serial.print(F("OC Voltage: "));
  Serial.print(voltage);
  Serial.println(F(" V"));
  Serial.print(F("Current: "));
  Serial.print(current);
  Serial.println(F(" A"));

  Serial.print(F("Predicted SOC (Capacity): "));
  Serial.print(prediction);
  Serial.print(F(" %"));
  Serial.print(F(" ("));
  Serial.print(prediction / 100 * 3.4);
  Serial.println(F(" Ah)"));
  Serial.println(F("-----------------"));

  SafetyMechanism(temperature, current, voltage);
  msg += String(current, 2) + "," + String(voltage, 2) + "," + String(temperature, 1) + "," + String(prediction, 1);
  Serial1.println(msg);

  delay(500);
}