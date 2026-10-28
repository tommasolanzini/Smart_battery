#include <Arduino.h>
#include <Adafruit_INA219.h>
#include "rf.h"
#include <WDT.h>
#include <EEPROM.h>
#include <Wire.h>

#define pinTemp1 A0
#define pinTemp2 A1
#define pinTemp3 A2
#define VREF_VOLT 4.65
#define VREF_TEMP 4.5


#define MinTemp -10
#define MaxTemp 70
#define MaxGradTemp 5
#define MaxDistTemp 5
#define MaxCurr 2
#define MinCurr -0.1
#define MaxGradCurr 0.5
#define MaxPen 30

// ----------------- INA219 Setup -----------------
Adafruit_INA219 ina219(0x44);

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

// Fault-Tolerance Variables
int idx1;
int idx2;
int mode = 0;
const int MAX_WDT_ATTEMPTS = 3;
const int EEPROM_WDT_COUNT_ADDR = 20;
const int EEPROM_flag2 = 19;

unsigned long lastLoopTime = 0;
const unsigned long SAFETY_TIMEOUT = 2000000;

// ------------- SAFE MODE FUNCTIONS -------------

void EnterSafeMode(int idx1, int idx2) {  //FUNZIONE DI EMERGENZA (POST-CRASH)
  Serial.println("ENTERING SAFE MODE (System Halted)-");
  if ((!idx1) && (!idx2)) {
    Serial.println("Cause: Previous Watchdog Timeout detected.");
  } else if (!idx2) {
    Serial.println("Cause: Faulty current sensor, number: ");
    Serial.print(idx1);
  } else {
    Serial.println("Cause: Faulty temperature sensors, number: ");
    Serial.print(idx1);
    Serial.print(" & ");
    Serial.print(idx2);
  }
  while (1) {
    delay(50);
    WDT.refresh();
  }
}

void checkResetReason() {  // LEGGE LA CAUSA DEL RESET
  uint16_t resetCause = R_SYSTEM->RSTSR1;
  byte wdtCount = 0;
  EEPROM.get(EEPROM_WDT_COUNT_ADDR, wdtCount);

  if (resetCause & ((1 << 1) | (1 << 0))) {
    Serial.println("!!! ALERT: SYSTEM REBOOTED BY WATCHDOG !!!");
    R_SYSTEM->RSTSR1 &= ~((1 << 1) | (1 << 0));  // Pulisce flag
    wdtCount++;                                  // Incrementa il contatore
    EEPROM.put(EEPROM_WDT_COUNT_ADDR, wdtCount);
    Serial.print("!!! ALERT: WATCHDOG RESET #");
    Serial.println(wdtCount);

    if (wdtCount >= MAX_WDT_ATTEMPTS) {
      EnterSafeMode(0, 0);
    } else {
      Serial.println("Attempting normal reboot...");
      R_SYSTEM->RSTSR1 &= ~((1 << 1) | (1 << 0));
    }
  } else {
    // SE L'AVVIO È NORMALE (Power-on o Reset manuale)
    Serial.println("System boot: Normal. Resetting crash counter.");
    wdtCount = 0;
    EEPROM.put(EEPROM_WDT_COUNT_ADDR, wdtCount);  // Azzera il contatore
  }
}

// ------------- ARDUINO SETUP -------------

void setup() {
  Serial.begin(9600);
  while (!Serial) { delay(10); }

  checkResetReason();  // Controlla perché ci siamo accesi

  Serial.println("ML test started");
  delay(50);

  if (!ina219.begin()) {
    Serial.println("INA219 not found");
    while (1)
      ;
  }
  ina219.setCalibration_32V_1A();
  Serial.println("INA219 ready");

  WDT.begin(2000);
  lastLoopTime = micros();  // Inizializza il timer
}

// ------------- FUNCTIONS -------------

void ReadTemperature(float* temp) {
  int raw1 = analogRead(pinTemp1);
  temp[0] = raw1 * 450 / 1023 - 50;
  int raw2 = analogRead(pinTemp2);
  temp[1] = raw2 * 450 / 1023 - 50;
  int raw3 = analogRead(pinTemp3);
  temp[2] = raw3 * 450 / 1023 - 50;
  return;
};

float average(float x, float x_avg) {
  return (x_avg * 10 + x) / 11;
};

float Voting(float* vec, int* faulty, int len, float* old_vec, int idx, int mode, float dist) {
  // SANITY
  if (len == 3) {
    float d[3];
    d[0] = fabs(vec[1] - vec[0]);
    d[1] = fabs(vec[1] - vec[2]);
    d[2] = fabs(vec[2] - vec[0]);
    float mindist = d[0];
    for (int i = 0; i < 3; i++) {
      if (d[i] < mindist) {
        mindist = d[i];
      }
    }
    if (mindist > dist) faulty += 2;

    bool fault;
    for (int i = 0; i < len; i++) {
      fault = vec[i] > MaxTemp) || (vec[i] < MinTemp) || (fabs(vec[i] - old_vec[i]) > MaxGradTemp);
      if (fault && faulty[i] < MaxPen) {
        faulty[i] += 2;
      } else if (faulty[i] > 0) {
        faulty[i] -= 1;
      }
      old_vec[i] = vec[i];
    }
  } else if (len == 2) {
    for (int i = 0; i < len; i++) {

      bool fault =
        (vec[i] > MaxCurr) || (vec[i] < MinCurr) || (fabs(vec[i] - old_vec[i]) > MaxGradCurr);

      if (fault && faulty[i] < MaxPen) {
        faulty[i] += 2;
      } else if (faulty[i] > 0) {
        faulty[i] -= 1;
      }
      old_vec[i] = vec[i];
    }
  }

  // VOTING
  float val = 0;
  switch (len) {
    case 3:
      {
        float weight[3];
        float d[3];
        d[0] = fabs(vec[1] - vec[0]);
        d[1] = fabs(vec[1] - vec[2]);
        d[2] = fabs(vec[2] - vec[0]);
        float tot_sum = d[0] + d[1] + d[2];
        if (tot_sum == 0) { return vec[0]; }
        weight[0] = d[1] / (tot_sum);
        weight[1] = d[2] / (tot_sum);
        weight[2] = d[0] / (tot_sum);

        float minweight = weight[0];
        float maxweight = weight[0];
        int minIdx = 0;
        int maxIdx = 0;

        for (int i = 0; i < 3; i++) {
          if (weight[i] < minweight) {
            minweight = weight[i];
            minIdx = i;
          }
        }

        if (minweight < 0.15) {
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
              if (faulty[minIdx] < MaxPen) {
                faulty[minIdx] = faulty[minIdx] + 2;
              }
              continue;
            }
            if (faulty[i]) {
              faulty[i] -= 1;
            }
          }
        } else {
          val = weight[0] * vec[0] + weight[1] * vec[1] + weight[2] * vec[2];
          for (int i = 0; i < 3; i++) {
            if (faulty[i]) {
              faulty[i] -= 1;
            }
          }
        }

        if (mode) {
          int j = 0;
          val = 0;
          for (int i = 0; i < len; i++) {
            if (i == idx) {
              continue;
            }
            val += vec[i] * 0.5;
            j++;
          }
        }
        Serial.print(faulty[0]); Serial.print(faulty[1]); Serial.println(faulty[2]);
        break;
      }
    case 2:
      {
        val = (vec[0] + vec[1]) / 2;
        break;
      }
  }
  return val;
}

void FaultHandling(int* faulty, int len, int idx1, int idx2, String type) {

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

  if (idx1) Serial.println("Degraded Mode");
  else Serial.println("Nominal Mode");
}

void loop() {
  // --- WD feeding ---
  WDT.refresh();
  lastLoopTime = micros();

  // --- Readings ---
  voltage = ina219.getBusVoltage_V();
  current = ina219.getCurrent_mA() / 1000.0;
  ReadTemperature(temp_vec);
  for (int i=0; i<3; i++){
    temp_vec[i] = average(temp_vec[i], OldTemp[i]);
  }

  // --- Voting & Fault Handling ---
  temperature = Voting(temp_vec, faulty_temp, 3, OldTemp, idx1, mode, MaxDistTemp);
  FaultHandling(faulty_temp, 3, idx1, idx2, "Temperature");
  

  // --- Create Input ---
  input[0] = voltage * 5e3;
  input[1] = temperature * 1e3;
  input[2] = current * (-10);

  // --- Predict ---
  prediction = RandomForest_predict(input, 3);

  Serial.print("Bus Voltage: ");
  Serial.print(voltage);
  Serial.println(" V");
  Serial.print("Current: ");
  Serial.print(current);
  Serial.println(" A");
  Serial.print("Temperature: ");
  Serial.print(temp_vec[0]);
  Serial.println(" °C");
  Serial.print("Temperature: ");
  Serial.print(temp_vec[1]);
  Serial.println(" °C");
  Serial.print("Temperature: ");
  Serial.print(temp_vec[2]);
  Serial.println(" °C");
  //Serial.print("Prediction: ");
  //Serial.print(prediction / 327);
  //Serial.println(" %");
  delay(50);
}