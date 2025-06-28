#include <AccelStepper.h>

// Pines motores
#define DIR_X 4
#define STEP_X 2
#define DIR_Y 14
#define STEP_Y 12

// Láser
#define LASER_PIN 26

// Crea los motores (driver tipo 1 = driver tipo pulso/dirección)
AccelStepper motorX(AccelStepper::DRIVER, STEP_X, DIR_X);
AccelStepper motorY(AccelStepper::DRIVER, STEP_Y, DIR_Y);

// Parámetros de movimiento
const int letterSpacing = 100;  // Espaciado entre letras
const int drawSpeed = 800;      // Velocidad de dibujo en pasos/segundo

String texto = "";

void setup() {
  Serial.begin(115200);

  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, LOW);

  motorX.setMaxSpeed(1000);
  motorX.setAcceleration(500);

  motorY.setMaxSpeed(1000);
  motorY.setAcceleration(500);
}

void loop() {
  if (Serial.available()) {
    texto = Serial.readStringUntil('\n');
    texto.trim();

    Serial.println("Recibido: " + texto);
    dibujarTexto(texto);
    Serial.println("FIN");
  }
}

// Dibuja texto carácter por carácter
void dibujarTexto(String txt) {
  for (int i = 0; i < txt.length(); i++) {
    char letra = txt[i];
    Serial.println("Dibujando letra: " + String(letra));
    dibujarLetra(letra);
    moverRelativo(motorX, letterSpacing);  // Espacio entre letras
  }
}

// Dibuja letras simples como líneas
void dibujarLetra(char letra) {
  digitalWrite(LASER_PIN, HIGH);

  switch (toupper(letra)) {
    case 'L':
      moverRelativo(motorY, 80);
      moverRelativo(motorY, -80);
      moverRelativo(motorX, 40);
      break;
    case 'U':
      moverRelativo(motorY, 80);
      moverRelativo(motorX, 30);
      moverRelativo(motorY, -80);
      break;
    case 'I':
      moverRelativo(motorY, 80);
      moverRelativo(motorY, -80);
      break;
    case 'S':
      moverRelativo(motorX, 40);
      moverRelativo(motorY, 40);
      moverRelativo(motorX, -40);
      moverRelativo(motorY, 40);
      moverRelativo(motorX, 40);
      break;
    default:
      break;
  }

  digitalWrite(LASER_PIN, LOW);
}

// Movimiento relativo con aceleración
void moverRelativo(AccelStepper &motor, int pasos) {
  long objetivo = motor.currentPosition() + pasos;
  motor.moveTo(objetivo);
  while (motor.distanceToGo() != 0) {
    motor.run();
  }
}
