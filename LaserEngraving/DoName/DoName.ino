#include <AccelStepper.h>

// Configuración de pines
#define STEP1 2
#define DIR1 4
#define STEP2 12
#define DIR2 14

// Crear objetos de motor
AccelStepper motorY(AccelStepper::DRIVER, STEP1, DIR1);  // Eje Y
AccelStepper motorX(AccelStepper::DRIVER, STEP2, DIR2);  // Eje X

// Parámetros de movimiento
const float MAX_SPEED = 1500.0;
const float ACCELERATION = 800.0;
const int STEPS_PER_MM = 80;  // Ajustar según tu configuración

// Variables de posición
long targetX = 0;
long targetY = 0;

void setup() {
  Serial.begin(115200);
  
  // Configurar motores
  motorX.setMaxSpeed(MAX_SPEED);
  motorX.setAcceleration(ACCELERATION);
  
  motorY.setMaxSpeed(MAX_SPEED);
  motorY.setAcceleration(ACCELERATION);
  
  // Establecer posiciones iniciales
  motorX.setCurrentPosition(0);
  motorY.setCurrentPosition(0);
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    
    if (input == "L") {
      // Levantar "lápiz" (retraer 5mm en Y)
      motorY.move(-5 * STEPS_PER_MM);
      while (motorY.distanceToGo() != 0) {
        motorY.run();
      }
    } else {
      // Parsear coordenadas
      int commaIndex = input.indexOf(',');
      if (commaIndex > 0) {
        targetX = input.substring(0, commaIndex).toInt();
        targetY = input.substring(commaIndex + 1).toInt();
        
        // Mover a nueva posición
        motorX.moveTo(targetX);
        motorY.moveTo(targetY);
        
        // Ejecutar movimientos
        while (motorX.distanceToGo() != 0 || motorY.distanceToGo() != 0) {
          motorX.run();
          motorY.run();
        }
      }
    }
  }
}