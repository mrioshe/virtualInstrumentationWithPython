void setup() {
  Serial.begin(9600);      // Iniciar comunicación serial
}

void loop() {
  int valor = analogRead(A0);     // Leer el valor analógico en A0 (0–1023)
  Serial.println(valor);          // Enviar el valor por el puerto serial
  delay(1);                      // Pequeña pausa para estabilizar la lectura (ajustable)
}