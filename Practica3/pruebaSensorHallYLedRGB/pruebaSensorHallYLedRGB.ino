// Pines LED RGB
const int pinRojo = 11;
const int pinVerde = 10;
const int pinAzul = 9;

// Pin del sensor Hall
const int pinHall = 8;

// Variable para guardar el estado anterior del Hall
int estadoHallAnterior = -1;

void setup() {
  // Configuración de pines de salida
  pinMode(pinRojo, OUTPUT);
  pinMode(pinVerde, OUTPUT);
  pinMode(pinAzul, OUTPUT);
  
  // Configuración del pin de entrada
  pinMode(pinHall, INPUT);

  // Inicialización del monitor serial
  Serial.begin(9600);
}

void loop() {
  int estadoHall = digitalRead(pinHall);

  // Solo imprimir si cambia el estado
  if (estadoHall != estadoHallAnterior) {
    if (estadoHall == HIGH) {
      Serial.println("Sensor Hall: ACTIVADO (1)");
    } else {
      Serial.println("Sensor Hall: DESACTIVADO (0)");
    }
    estadoHallAnterior = estadoHall;
  }

  if (estadoHall == HIGH) {
    // Morado = rojo + azul
    digitalWrite(pinRojo, HIGH);
    digitalWrite(pinVerde, LOW);
    digitalWrite(pinAzul, HIGH);
  } else {
    // Aguamarina = verde + azul
    digitalWrite(pinRojo, LOW);
    digitalWrite(pinVerde, HIGH);
    digitalWrite(pinAzul, HIGH);
  }
}

