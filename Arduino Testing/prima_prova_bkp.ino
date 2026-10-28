#include <Arduino.h>
#include <Adafruit_INA219.h>
#include "random_forest_micro.h"
// #include <WDT.h> // libreria per watchdogs
// #include <EEPROM.h> // aggiunta per il WD

#define pinTemp1 A0
#define pinTemp2 A1
#define VREF 4.5

// Oggetto del modello ML
Eloquent::ML::Port::RandomForestRegressor model;

// ----------------- INA219 Setup -----------------
Adafruit_INA219 ina219;


// ------------- ARDUINO SETUP -------------
// volatile int WDTset = 1; // WDT

void setup() {
  Serial.begin(9600);

  while (!Serial) {
    delay(10); // attende connessione seriale
  }
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

// Definition of mean and standard deviation
const float mean[4] = {3.6014138, -0.98064475, 8.72646614, 3.60623397};
const float stdv[4] = {0.28740054, 0.76967008, 14.27595781, 0.26602317};
void normalize(float *x, float *x_norm, int n_features){
    for(int i=0; i < n_features; i++){
        x_norm[i] = (x[i] - mean[i])/stdv[i];
    }
}

float raw1; float raw2; float temp1; float temp2;
float ReadTemperature(){
    raw1 = (analogRead(pinTemp1)/1023.0) * VREF;
    raw2 = (analogRead(pinTemp2)/1023.0) * VREF;
    temp1 = (raw1 - 0.5) * 100.0;
    temp2 = (raw2 - 0.5) * 100.0;
    return (temp1 + temp2) / 2;
};

float average(float x, float x_avg){
    return (x_avg*10 + x) / 11;
};

// WDT function per capire se il WD è stato tilizzato (ha resettato)
// void getSettings() {
//   Serial.begin(57600);
//   // Sulla R4 è bene attendere che la seriale sia pronta (essendo USB nativa)
//   while (!Serial); 

//   Serial.println("We just started up......");
  
//   byte b;
//   // Leggiamo il valore all'indirizzo 10
//   EEPROM.get(10, b); 

//   // Corretto l'errore logico: controlliamo se b è effettivamente segnalato
//   if (b == 1) { 
//     Serial.println("...and a Watchdog timeout occurred before this reset!");
//     // questo oppure come consiglia jackie un bel led che si accende
//     // Resettiamo il flag in EEPROM
//     b = 0;
//     EEPROM.put(10, b);
//   }
// }

// ------------- VARIABLES -------------
float input[4];
float input_norm[4];
float voltage;
float current;
float temperature;
float voltage_avg;
int flag = 0;

void loop() {

    // --- Read INA219 ---
    voltage = ina219.getBusVoltage_V();       // Voltage on load
    current = ina219.getCurrent_mA()/1000.0;  // Correct A

    // --- Read TMP36 ---
    temperature = ReadTemperature();          // Temperature on load

    if(!flag){
        voltage_avg = voltage;
        flag++;
    }
    else{
        voltage_avg = average(voltage, voltage_avg);
    };

    // --- Create Input ---
    input[0] = voltage;
    input[1] = current;
    input[2] = temperature;
    input[3] = voltage_avg;

    // --- Normalize ---
    normalize(input, input_norm, 4);

    // --- Predict ---
    float prediction = model.predict(input_norm);

    // --- Print all results ---
    Serial.print("Bus Voltage: "); Serial.print(voltage); Serial.println(" V");
    Serial.print("Current: "); Serial.print(current); Serial.println(" A");
    Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" °C");
    Serial.print("Prediction: "); Serial.print(prediction*100); Serial.println(" %");
    Serial.println("------------------------");


    delay(50);
}
