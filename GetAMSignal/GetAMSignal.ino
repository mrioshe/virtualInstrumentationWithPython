// ============================================
// LECTOR DE SEÑAL AM - ARDUINO
// ============================================
// Lee señal analógica y envía valores por serial

const int pinEntrada = A0;  // Pin donde llega la señal analógica
const unsigned long intervalo_us = 200; // 200 us = 5 kHz (suficiente para 1kHz portadora)

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Esperar a que se abra el puerto serial
  }
  analogReference(DEFAULT); // 5V de referencia
  pinMode(pinEntrada, INPUT);
}

void loop() {
  static unsigned long ultimoTiempo = 0;
  unsigned long tiempoActual = micros();

  // Muestreo a intervalo fijo
  if (tiempoActual - ultimoTiempo >= intervalo_us) {
    ultimoTiempo = tiempoActual;
    
    // Leer valor analógico (0-1023)
    int valor = analogRead(pinEntrada);
    
    // Enviar inmediatamente
    Serial.println(valor);
  }
}