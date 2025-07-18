#include <AccelStepper.h>

// Pines
#define STEP_Y 2
#define DIR_Y 4
#define STEP_X 12
#define DIR_X 14
const int laserPin = 27;

// Motores
AccelStepper motorY(AccelStepper::DRIVER, STEP_Y, DIR_Y);
AccelStepper motorX(AccelStepper::DRIVER, STEP_X, DIR_X);

// Configuración
const int STEPS_PER_MM = 80; // Ajusta según tu hardware
const float LETTER_SIZE = 20.0; // 2x2 cm (20mm x 20mm)
const float SPACING = 5.0; // Espacio entre letras (5mm)

void setup() {
  Serial.begin(115200);
  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, LOW);
  
  motorX.setMaxSpeed(1500);
  motorY.setMaxSpeed(1500);
  motorX.setAcceleration(800);
  motorY.setAcceleration(800);
  
  Serial.println("Listo para recibir nombres...");
}

void moveTo(float x, float y, bool laserOn) {
  digitalWrite(laserPin, laserOn ? HIGH : LOW);
  motorX.moveTo(x * STEPS_PER_MM);
  motorY.moveTo(y * STEPS_PER_MM);
  while (motorX.run() || motorY.run()) delayMicroseconds(100);
}

void drawLetter(char letter, float startX, float startY) {
  switch (toupper(letter)) {
    case 'A':
      moveTo(startX, startY, false);
      moveTo(startX, startY + LETTER_SIZE, true);
      moveTo(startX + LETTER_SIZE, startY + LETTER_SIZE, true);
      moveTo(startX + LETTER_SIZE, startY, true);
      moveTo(startX, startY + LETTER_SIZE/2, true);
      moveTo(startX + LETTER_SIZE, startY + LETTER_SIZE/2, true);
      break;
      
    case 'B':
      moveTo(startX, startY, false);
      moveTo(startX, startY + LETTER_SIZE, true);
      moveTo(startX + LETTER_SIZE*0.7, startY + LETTER_SIZE, true);
      moveTo(startX + LETTER_SIZE, startY + LETTER_SIZE*0.7, true);
      moveTo(startX + LETTER_SIZE, startY + LETTER_SIZE*0.3, true);
      moveTo(startX + LETTER_SIZE*0.7, startY, true);
      moveTo(startX, startY, true);
      break;
      
    // Añade más letras aquí...
      
    default:
      break;
  }
}

void writeName(String name) {
  float x = 50; // Posición inicial X (50mm desde el borde)
  float y = 50; // Posición inicial Y (50mm desde el borde)
  
  for (int i = 0; i < name.length(); i++) {
    drawLetter(name[i], x, y);
    x += LETTER_SIZE + SPACING; // Mover a la siguiente posición
  }
  
  moveTo(0, 0, false); // Regresar al origen
  Serial.println("Nombre escrito!");
}

void loop() {
  if (Serial.available()) {
    String name = Serial.readStringUntil('\n');
    name.trim();
    writeName(name);
  }
}