const int pwmPin = 27;
const int frecuencia = 5000;         // Frecuencia deseada (en Hz)
const float ciclo_util = 0.5;       // Duty cycle: 0.0 a 1.0 (50% en este caso)

void setup() {
  pinMode(pwmPin, OUTPUT);
}

void loop() {
  // Calcular período total en microsegundos
  int periodo_us = 1000000 / frecuencia;

  // Tiempo en alto y en bajo
  int tiempo_alto = periodo_us * ciclo_util;
  int tiempo_bajo = periodo_us - tiempo_alto;

  // Generar PWM manual
  digitalWrite(pwmPin, HIGH);
  delayMicroseconds(tiempo_alto);
  digitalWrite(pwmPin, LOW);
  delayMicroseconds(tiempo_bajo);
}

