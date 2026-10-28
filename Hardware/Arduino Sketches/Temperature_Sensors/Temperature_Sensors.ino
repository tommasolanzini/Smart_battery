// TMP36 Temperature Sensors

#define TempSensor1 A0  // First sensor connected to analog pin A0
#define TempSensor2 A1  // Second sensor connected to analog pin A1
#define N 100            // Number of measurements per cycle
#define VREF 1.125        // Internal reference voltage (Arduino INTERNAL = 1.1V)

void setup() {
  Serial.begin(9600);           // Initialize serial communication
  analogReference(INTERNAL);    // Use internal 1.1V reference for higher resolution and stability
}



// Function: readSensor
// Reads a sensor N times, converts voltage to Celsius, stores in array
void readSensor(float values[], int pin) {
  for (int i = 0; i < N; i++) {
    // Read analog value, convert to voltage using internal reference
    float voltage = analogRead(pin) * (VREF / 1023.0);
    
    // Convert voltage to temperature (TMP36 formula) - 0.5 is the offset (look to DS)
    // Temp (°C) = (Voltage - 0.5) * 100
    values[i] = (voltage - 0.5) * 100.0;

    delay(10);  // Small delay to stabilize readings
  }
}

// Function: calculateMean
// Calculates the mean of an array of floats
float calculateMean(float values[]) {
  float sum = 0;
  for (int i = 0; i <N ; i++) {
    sum += values[i];
  }
  return sum / N;
}

// Function: calculateStdDev
// Calculates the standard deviation of an array of floats
float calculateStdDev(float values[], float mean) {
  float sum = 0;
  for (int i = 0; i < N; i++) {
    float diff = values[i] - mean;
    sum += diff * diff;
  }
  return sqrt(sum / N);
}

void loop() {

  // Allocating
  float values1[N], values2[N];

  // Read both sensors
  readSensor(values1, TempSensor1);
  readSensor(values2, TempSensor2);

  // Calculate mean temperatures
  float mean1 = calculateMean(values1);
  float mean2 = calculateMean(values2);

  // Calculate standard deviation
  float stdDev1 = calculateStdDev(values1, mean1);
  float stdDev2 = calculateStdDev(values2, mean2);

  // Calculate weights inversely proportional to deviation
  // The more stable sensor contributes more to the final temperature
  float weight1 = stdDev2 / (stdDev1 + stdDev2);
  float weight2 = stdDev1 / (stdDev1 + stdDev2);

  // Weighted average of the two sensors
  float finalTemperature = mean1 * weight1 + mean2 * weight2;

  // Print results
  Serial.print("Temperature: "); Serial.print(finalTemperature); Serial.println (" °C");
  Serial.println("------------------------------");

  delay(1000);  // Wait 1 second before next measurement
}
