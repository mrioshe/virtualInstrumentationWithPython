// Usa GPIO25 (DAC1) para salida analógica
#define LED 2
const int dacPin = 25;

String inputString = "";
bool receiving = false;

void setup() {
  Serial.begin(115200);  // Debe coincidir con el baud rate en Python
  pinMode(LED, OUTPUT);
}

void loop() {
  digitalWrite(LED, LOW);
  while (Serial.available()) {
    digitalWrite(LED, HIGH);
    char c = Serial.read();
    
    if (c == '\n') {
      float valorVoltaje = inputString.toFloat();  // Convertir string a float

      // Asegurarse que el voltaje no exceda 3.3 V (el máximo del DAC)
      valorVoltaje = constrain(valorVoltaje, 0.0, 3.3);

      // Convertir voltaje (0 a 3.3 V) a valor de DAC (0 a 255)
      int valorDAC = mapFloat(valorVoltaje, 0.0, 3.3, 0, 255);
      
      dacWrite(dacPin, valorDAC);

      inputString = "";  // Limpiar para el siguiente valor
    } else {
      inputString += c;  // Acumular caracteres
    }
  }
}

// Función personalizada para mapear float a int
int mapFloat(float x, float in_min, float in_max, int out_min, int out_max) {
  return (int)((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}