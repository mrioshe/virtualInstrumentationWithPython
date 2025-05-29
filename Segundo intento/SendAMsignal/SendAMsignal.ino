#include "driver/dac.h"

#define DAC_PIN 25      // GPIO25 (DAC1)
#define BUFFER_SIZE 512 // Buffer circular
#define SAMPLE_DELAY 200 // Microsegundos (5kHz)

byte rxBuffer[BUFFER_SIZE];
volatile uint16_t bufIndex = 0;
bool dataReady = false;

void setup() {
  Serial.begin(921600);
  while(!Serial); 
  
  // Configurar DAC
  dac_output_enable(DAC_CHANNEL_1);
  pinMode(DAC_PIN, OUTPUT);
  
  Serial.println("ESP32 Listo para recibir AM @800Hz");
}

void loop() {
  // Recepción serial no bloqueante
  while(Serial.available()) {
    rxBuffer[bufIndex] = Serial.read();
    bufIndex = (bufIndex + 1) % BUFFER_SIZE;
    dataReady = true;
  }
  
  // Reproducción constante desde buffer
  if(dataReady) {
    dac_output_voltage(DAC_CHANNEL_1, rxBuffer[bufIndex]);
    delayMicroseconds(SAMPLE_DELAY);
    bufIndex = (bufIndex + 1) % BUFFER_SIZE;
    }
}