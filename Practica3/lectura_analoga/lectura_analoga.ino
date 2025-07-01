void setup() {
  Serial.begin(9600); // Inicia comunicación serial a 9600 baudios
}

void loop() {
  int valor = analogRead(A0); // Leer el valor del pin A0 (0 a 1023)
  Serial.println(valor);      // Mostrar el valor en el monitor serial
  delay(200);                 // Espera de 200 ms entre lecturas
}
