#define LED_BUILTIN 2  // Pin 2 suele estar conectado al LED en el ESP32

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);  // Configura el pin como salida
}
void loop() {
  digitalWrite(LED_BUILTIN, HIGH);  // Enciende el LED
  delay(500);                       // Espera 500 ms
  digitalWrite(LED_BUILTIN, LOW);   // Apaga el LED
  delay(500);                       // Espera 500 ms
}