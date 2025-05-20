const int pinEntrada = A0;  // Pin donde llega la señal analógica
const unsigned long tasaMuestreo_us = 2000; // 1000 us = 1 ms = 1000 Hz

void setup() {
  Serial.begin(115200);  // 115200 es más rápido y estable si se usa bien
}

void loop() {
  static unsigned long t_anterior = 0;
  unsigned long t_actual = micros();

  if (t_actual - t_anterior >= tasaMuestreo_us) {
    t_anterior = t_actual;
    
    int valor = analogRead(pinEntrada);  // 0 a 1023
    Serial.println(valor);  // Enviar por serial como string
  }
}