#include <AccelStepper.h>

// Pines de los motores
#define STEP1 2
#define DIR1 4
#define STEP2 12
#define DIR2 14

// PWM manual (láser)
const int pwmPin = 27;
const int frecuencia = 5000;       // Frecuencia en Hz
const float ciclo_util = 0.5;      // Duty cycle: 50%

// Crear objetos para motores
AccelStepper motorY(AccelStepper::DRIVER, STEP1, DIR1);  // Eje Y
AccelStepper motorX(AccelStepper::DRIVER, STEP2, DIR2);  // Eje X

// Configuración de los motores
const float MAX_SPEED = 1500.0;
const float ACCELERATION = 800.0;
const int STEPS_PER_MM = 80;

// Variables de posición
long targetX = 0;
long targetY = 0;

bool dibujandoLetra = false;

void setup() {
  Serial.begin(115200);

  pinMode(pwmPin, OUTPUT);
  digitalWrite(pwmPin, LOW);  // Láser apagado al inicio

  // Motores
  motorX.setMaxSpeed(MAX_SPEED);
  motorX.setAcceleration(ACCELERATION);
  motorX.setCurrentPosition(0);

  motorY.setMaxSpeed(MAX_SPEED);
  motorY.setAcceleration(ACCELERATION);
  motorY.setCurrentPosition(0);
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input == "L") {
      apagarLaser();
      // Simula levantar lápiz bajando 5mm en Y
      motorY.move(-5 * STEPS_PER_MM);
      while (motorY.distanceToGo() != 0) {
        motorY.run();
      }
    } else {
      // Recibimos un nombre completo (como "Luis")
      Serial.println("Recibido: " + input);
      dibujarTexto(input);
    }
  }
}

// Dibuja una palabra completa
void dibujarTexto(String texto) {
  for (int i = 0; i < texto.length(); i++) {
    char letra = texto[i];
    Serial.println("Dibujando letra: " + String(letra));
    dibujarLetra(letra);
    
    // Mover a la derecha para siguiente letra (láser apagado)
    apagarLaser();
    moverRelativo(motorX, 30);  // Espaciado horizontal entre letras
  }

  apagarLaser();  // Apagar láser al final
  Serial.println("FIN");
}

// Simula dibujo de una letra (ejemplo simple)
void dibujarLetra(char letra) {
  letra = toupper(letra);
  
  if (letra == 'L') {
    moverConLaser(0, 20);  // Subir
    moverConLaser(0, -20); // Bajar
    moverConLaser(20, 0);  // Derecha
  } else if (letra == 'I') {
    moverConLaser(0, 30);
    moverConLaser(0, -30);
  } else if (letra == 'S') {
    moverConLaser(20, 0);
    moverConLaser(0, 20);
    moverConLaser(-20, 0);
    moverConLaser(0, 20);
    moverConLaser(20, 0);
  } else {
    // Si la letra no está definida, no hacer nada
  }
}

// Movimiento relativo simple
void moverRelativo(AccelStepper &motor, int pasos) {
  long destino = motor.currentPosition() + pasos;
  motor.moveTo(destino);
  while (motor.distanceToGo() != 0) {
    motor.run();
  }
}

// Movimiento relativo de ambos motores con láser activo
void moverConLaser(int dx_mm, int dy_mm) {
  long destinoX = motorX.currentPosition() + dx_mm * STEPS_PER_MM;
  long destinoY = motorY.currentPosition() + dy_mm * STEPS_PER_MM;

  motorX.moveTo(destinoX);
  motorY.moveTo(destinoY);

  encenderLaser();

  while (motorX.distanceToGo() != 0 || motorY.distanceToGo() != 0) {
    motorX.run();
    motorY.run();

    generarPWM();  // PWM solo mientras se mueve
  }

  apagarLaser();
}

// PWM por software (manual)
void generarPWM() {
  int periodo_us = 1000000 / frecuencia;
  int tiempo_alto = periodo_us * ciclo_util;
  int tiempo_bajo = periodo_us - tiempo_alto;

  digitalWrite(pwmPin, HIGH);
  delayMicroseconds(tiempo_alto);
  digitalWrite(pwmPin, LOW);
  delayMicroseconds(tiempo_bajo);
}

// Encender/apagar láser
void encenderLaser() {
  dibujandoLetra = true;
}

void apagarLaser() {
  dibujandoLetra = false;
  digitalWrite(pwmPin, LOW);
}
