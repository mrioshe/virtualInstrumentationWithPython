// Pines motores
const int STEP1_PIN = 2;
const int DIR1_PIN  = 4;
const int STEP2_PIN = 12;
const int DIR2_PIN  = 14;

// Parámetros del movimiento
const int stepsPerMovement = 400;  // Ajusta según tus motores
const int stepDelay = 1000;        // Microsegundos entre pasos (1000 us = 1 ms = ~1000 pasos/seg)

void setup() {
  pinMode(STEP1_PIN, OUTPUT);
  pinMode(DIR1_PIN, OUTPUT);
  pinMode(STEP2_PIN, OUTPUT);
  pinMode(DIR2_PIN, OUTPUT);
}

void loop() {
  // Movimiento 1: Izquierda a derecha (Motor X adelante)
  digitalWrite(DIR1_PIN, HIGH);
  moverMotor(STEP1_PIN, stepsPerMovement);

  delay(1000); // Pausa entre movimientos

  // Movimiento 2: Derecha a izquierda (Motor X atrás)
  digitalWrite(DIR1_PIN, LOW);
  moverMotor(STEP1_PIN, stepsPerMovement);

  delay(1000);

  // Movimiento 3: Arriba a abajo (Motor Y adelante)
  digitalWrite(DIR2_PIN, HIGH);
  moverMotor(STEP2_PIN, stepsPerMovement);

  delay(1000);

  // Movimiento 4: Abajo a arriba (Motor Y atrás)
  digitalWrite(DIR2_PIN, LOW);
  moverMotor(STEP2_PIN, stepsPerMovement);

  delay(1000);
}

// Función para mover un motor
void moverMotor(int stepPin, int steps) {
  for (int i = 0; i < steps; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(stepDelay);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(stepDelay);
  }
}
