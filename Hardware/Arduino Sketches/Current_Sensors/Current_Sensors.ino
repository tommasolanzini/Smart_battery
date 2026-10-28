#include <Wire.h>             // Include the Wire library for I2C communication
#include <Adafruit_INA219.h>  // Include Adafruit INA219 library for current/power sensor

Adafruit_INA219 ina219(0x44);;       // Create an INA219 object

void setup() {
  Serial.begin(9600);         // Start serial communication at 9600 baud
  while (!Serial) { delay(10); }  // Wait for Serial port to be ready (important for some boards)

  Serial.println("INA219 - Current measurement with 0.2 ohm shunt");

  // Initialize the INA219 sensor
  if (!ina219.begin()) {
    Serial.println("Error: INA219 not found!");  // Print error if sensor is not detected
    while (1) { delay(10); }                     // Stop program if sensor is missing
  }

  // Use default calibration for 32V / 2A (assumes 0.1 ohm shunt)
  ina219.setCalibration_32V_2A(); 
  Serial.println("INA219 ready");
}

void loop() {
  float busVoltage = ina219.getBusVoltage_V();      // Read bus voltage (voltage on the load)
  float shuntVoltage = ina219.getShuntVoltage_mV(); // Read voltage across shunt resistor in mV

  // Correct the current reading for your 0.2 ohm shunt
  // The default calibration assumes 0.1 ohm, so we multiply by 2
  float current_mA = ina219.getCurrent_mA() * 2.0; 

  float power_mW = busVoltage * current_mA;        // Calculate power in milliwatts

  // Print values to Serial Monitor
  Serial.print("Bus Voltage: ");
  Serial.print(busVoltage);
  Serial.println(" V");

  Serial.print("Current: ");
  Serial.print(current_mA);
  Serial.println(" mA");

  Serial.print("Power: ");
  Serial.print(power_mW);
  Serial.println(" mW");

  Serial.println("------------------------");  // Separator for readability

  delay(1000);  // Wait 1 second before next measurement
}
