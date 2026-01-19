const int pwmPin = 9; 
const int enablePin = 8;

int pwmIntensity = 120;

// CÁLCULO PARA 50 KHZ USANDO PRESCALER 8:
// El Prescaler 8 reduce el reloj del Timer de 16MHz a 2MHz.
// Frecuencia = (Reloj del Timer) / (TOP + 1)
// TOP = (Reloj del Timer / Frecuencia) - 1
// TOP = (2,000,000 / 50,000) - 1 = 39
const unsigned int PWM_TOP = 39;

void setup() {
  pinMode(pwmPin, OUTPUT);
  pinMode(enablePin, OUTPUT);
  
  digitalWrite(enablePin, HIGH);

  // --- CONFIGURACIÓN DE REGISTROS PARA 50 KHZ ---
  noInterrupts();
  
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  // Modo 14 (Fast PWM con ICR1 como TOP)
  TCCR1A |= (1 << COM1A1) | (1 << WGM11);
  TCCR1B |= (1 << WGM13) | (1 << WGM12);

  // **** CAMBIO CLAVE: Prescaler = 8 ****
  // CS11 = 1 para seleccionar el Prescaler 8
  TCCR1B |= (1 << CS11); 

  // Establecer el techo (TOP) para 50 kHz
  ICR1 = PWM_TOP;

  setPwmDuty(pwmIntensity);

  interrupts();
  // ---------------------------------------------

  Serial.begin(9600);
  Serial.println("PWM 50 KHZ CONTINUO ACTIVADO (EN PIN 9)");
  Serial.println("USANDO PRESCALER 8");
  Serial.print("Intensidad inicial: ");
  Serial.println(pwmIntensity);
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("INT ")) {
      int newInt = input.substring(4).toInt();
      if (newInt >= 0 && newInt <= 255) {
        pwmIntensity = newInt;
        setPwmDuty(pwmIntensity);
        Serial.print("Nueva intensidad PWM: ");
        Serial.println(pwmIntensity);
      }
    }
  }
}

void setPwmDuty(int value) {
  // Mapeamos 0-255 al nuevo rango de 0-39
  // Nota: La resolución es menor (solo 40 pasos), pero necesaria para 50kHz.
  OCR1A = map(value, 0, 255, 0, PWM_TOP);
}