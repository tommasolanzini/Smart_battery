#include <Arduino.h>
#define Temp A1
#define VREF 4.5

void setup(){
  Serial.begin(9600);
  while(!Serial);
  analogReadResolution(10);
}

void loop(){
  int raw = analogRead(Temp);
  Serial.println(raw);

  float voltage = VREF * (analogRead(Temp) / 1023.0);
  Serial.println(voltage, 3);

  float temperature = (voltage - 0.5) * 100.0;
  Serial.println(temperature, 3);

  delay(100);
}