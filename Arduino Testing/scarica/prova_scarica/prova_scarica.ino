#include <Arduino.h>
#include <Adafruit_INA219.h>
#include "random_forest_micro.h"
#include <WDT.h> 
#include <EEPROM.h> 

#define pinTemp1 A0
#define pinTemp2 A1
#define VREF 4.5 
// #define Pin_Voltage A5


// Oggetto del modello ML
Eloquent::ML::Port::RandomForestRegressor model;

// ----------------- INA219 Setup -----------------
Adafruit_INA219 ina219;



float input[4];
float input_norm[4];
float count[100];
float voltage;
float current;
float temperature;
float voltage_avg;
int count2 = 0;
int flag = 0;
const int EEPROM_WDT_COUNT_ADDR = 20; 


// ------------- ARDUINO SETUP -------------

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  while (!Serial) { delay(10); }

  Serial.println("ML test started");
  delay(50);

  // if (!ina219.begin()) {
  //   Serial.println("Error: INA219 not found!");
  //   while (1) { delay(10); }
  // }
  ina219.setCalibration_32V_1A(); 
  Serial.println("INA219 ready");

}

// ------------- FUNCTIONS -------------
const float mean[4] = {3.6014138, -0.98064475, 8.72646614, 3.60623397};
const float stdv[4] = {0.28740054, 0.76967008, 14.27595781, 0.26602317};

void normalize(float *x, float *x_norm, int n_features){
    for(int i=0; i < n_features; i++){
        x_norm[i] = (x[i] - mean[i])/stdv[i];
    }
}

float average(float x, float x_avg){
    return (x_avg*10 + x) / 11;
};

float ReadTemperature(){
  float raw1; float raw2; float temp1; float temp2;
  raw1 = (analogRead(pinTemp1)/1023.0) * VREF;
  raw2 = (analogRead(pinTemp2)/1023.0) * VREF;
  temp1 = (raw1 - 0.5) * 100.0;
  temp2 = (raw2 - 0.5) * 100.0;
  float val = (temp1 + temp2)/2; 
  return(val);
};

void loop() {

    voltage = 1;   
    // voltage = (analogRead(Pin_Voltage)/1023.0) * VREF;
    current = 1; 
    temperature = ReadTemperature(); 
    if(!flag){
        voltage_avg = voltage;
        flag++;
    }
    else{
        voltage_avg = average(voltage, voltage_avg);
    };

    input[0] = voltage;
    input[1] = current;
    input[2] = temperature;
    input[3] = voltage_avg;

    normalize(input, input_norm, 4);
    float prediction = model.predict(input_norm);
    Serial1.print(voltage);          
    Serial1.print(",");
    Serial1.print(current);
    Serial1.print(",");
    Serial1.println(prediction * 100);
    Serial.print("Bus Voltage: "); Serial.print(voltage); Serial.println(" V");
    Serial.print("Current: "); Serial.print(current); Serial.println(" A");
    Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" °C");
    Serial.print("Prediction: "); Serial.print(prediction*100); Serial.println(" %");

    Serial.println("------------------------");
    delay(50);

}