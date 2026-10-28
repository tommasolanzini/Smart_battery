
void setup() {
  Serial.begin(115200);    
  Serial1.begin(9600);    

  Serial.println("Test numeri Bluetooth avviato");
}

/*
void loop() {
  // Valori finti (simulazione sensori)
  float corrente = random(100, 300) / 100.0;   // 1.00 - 3.00 A
  float tensione = random(110, 130) / 10.0;    // 11.0 - 13.0 V
  float soc      = random(600, 1000) / 10.0;   // 60 - 100 %
  float temp     = random(200, 350) / 10.0;    // 20 - 35 °C

  // Determina mode
  String mode;
  if (soc < 20) {
    mode = "LowBattery";
  } else if (corrente > 0) {
    mode = "Discharging";
  } else {
    mode = "Charging";
  }
  */


  String msg =
    String(current, 2) + "," +
    String(voltage, 2) + "," +
    String(soc, 1) + "," +
    String(temp, 1) + "," +
    String mode;


  Serial1.println(msg);
  Serial.println(msg);

  delay(100);
}
