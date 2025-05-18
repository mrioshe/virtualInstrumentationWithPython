#define LED 2

void setup() {
  pinMode(LED, OUTPUT);
}

void loop() {
  digitalWrite(LED, HIGH);   // Encender LED
  delay(500);
  digitalWrite(LED, LOW);    // Apagar LED
  delay(500);
}