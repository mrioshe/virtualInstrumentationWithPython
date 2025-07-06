#include <AccelStepper.h>

// Configuración de pines motores
#define STEP1 2
#define DIR1 4
#define STEP2 12
#define DIR2 14

// PWM manual para láser
const int laserPin = 27;
const int frecuencia = 5000;          // 5 kHz
const float dutyCycle = 0.5;          // 50%
const unsigned long periodo = 200;     // 200 μs (1/5000Hz)

// Crear objetos de motor
AccelStepper motorY(AccelStepper::DRIVER, STEP1, DIR1);  // Eje Y
AccelStepper motorX(AccelStepper::DRIVER, STEP2, DIR2);  // Eje X

// Parámetros de movimiento
const float MAX_SPEED = 1500.0;
const float ACCELERATION = 800.0;
const int STEPS_PER_MM = 80;

// Variables de estado PWM
unsigned long prevMicros = 0;
bool laserState = LOW;
bool laserEnabled = false;

void setup() {
  Serial.begin(115200);
  
  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, LOW);

  // Configurar motores
  motorX.setMaxSpeed(MAX_SPEED);
  motorX.setAcceleration(ACCELERATION);
  motorY.setMaxSpeed(MAX_SPEED);
  motorY.setAcceleration(ACCELERATION);

  motorX.setCurrentPosition(0);
  motorY.setCurrentPosition(0);
}

void loop() {
  // Manejar PWM láser no bloqueante
  handleLaserPWM();
  
  // Manejar movimiento de motores
  bool isMoving = motorX.run() || motorY.run();
  
  // Procesar comandos seriales solo cuando no hay movimiento
  if (!isMoving && Serial.available()) {
    processSerialCommand();
  }
}

void handleLaserPWM() {
  if (!laserEnabled) return;
  
  unsigned long currentMicros = micros();
  unsigned long elapsed = currentMicros - prevMicros;
  
  if (laserState == HIGH && elapsed >= periodo * dutyCycle) {
    digitalWrite(laserPin, LOW);
    laserState = LOW;
    prevMicros = currentMicros;
  }
  else if (laserState == LOW && elapsed >= periodo * (1 - dutyCycle)) {
    digitalWrite(laserPin, HIGH);
    laserState = HIGH;
    prevMicros = currentMicros;
  }
}

void processSerialCommand() {
  String input = Serial.readStringUntil('\n');
  input.trim();

  if (input == "L") {  // Levantar lápiz
    laserEnabled = false;
    digitalWrite(laserPin, LOW);
    motorY.move(-5 * STEPS_PER_MM);
  }
  else {
    int commaIndex = input.indexOf(',');
    if (commaIndex > 0) {
      long x = input.substring(0, commaIndex).toInt();
      long y = input.substring(commaIndex + 1).toInt();
      
      motorX.moveTo(x);
      motorY.moveTo(y);
      
      // Preparar PWM para siguiente ciclo
      laserEnabled = true;
      laserState = LOW;
      prevMicros = micros();
    }
  }
}