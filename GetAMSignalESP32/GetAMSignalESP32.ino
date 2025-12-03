// ============================================
// GENERADOR AM DE ALTA FRECUENCIA - ESP32
// ============================================
// Genera señal AM hasta 10kHz usando Timer hardware
// DAC a 8 bits (0-255) -> 0-3.3V

#define LED 2
#define DAC_PIN 25

// Variables globales de la señal
volatile float fc = 1000;        // Frecuencia portadora (Hz)
volatile float fm = 50;          // Frecuencia moduladora (Hz)
volatile float m = 0.5;          // Índice de modulación
volatile float offset_v = 1.65;  // Offset DC (V)
volatile float amplitud = 1.0;   // Amplitud (V)

volatile bool generando = false;
volatile uint32_t sample_count = 0;

// Timer
hw_timer_t *timer = NULL;

// Frecuencia de muestreo (debe ser al menos 10x la portadora)
const uint32_t SAMPLE_RATE = 50000;  // 50kHz

// ============================================
// INTERRUPCIÓN DEL TIMER (ISR)
// ============================================
void IRAM_ATTR onTimer() {
  if (!generando) return;
  
  // Calcular tiempo actual
  float t = (float)sample_count / SAMPLE_RATE;
  
  // Generar señal AM: s(t) = Ac(1 + m*sin(2πfm*t)) * sin(2πfc*t)
  float moduladora = m * sin(2.0 * PI * fm * t);
  float portadora = sin(2.0 * PI * fc * t);
  float senal_am = amplitud * (1.0 + moduladora) * portadora;
  
  // Agregar offset y convertir a voltaje (0-3.3V)
  float voltaje = senal_am + offset_v;
  voltaje = constrain(voltaje, 0.0, 3.3);
  
  // Convertir a valor DAC (0-255)
  uint8_t dac_value = (uint8_t)((voltaje / 3.3) * 255.0);
  
  // Escribir al DAC
  dacWrite(DAC_PIN, dac_value);
  
  sample_count++;
}

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  
  // Configurar DAC
  dacWrite(DAC_PIN, 127);  // Punto medio inicial
  
  // Configurar Timer (nueva API ESP32 Core 2.x+)
  timer = timerBegin(SAMPLE_RATE);  // Frecuencia directa en Hz
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, 1000000 / SAMPLE_RATE, true, 0);  // Periodo en microsegundos
  
  Serial.println("\n=================================");
  Serial.println("  GENERADOR AM - ESP32");
  Serial.println("=================================");
  Serial.println("Comandos:");
  Serial.println("  START,fc,fm,m,offset,amp");
  Serial.println("  STOP");
  Serial.println("=================================\n");
  Serial.print("Sample Rate: ");
  Serial.print(SAMPLE_RATE);
  Serial.println(" Hz");
  Serial.println("Esperando comandos...\n");
}

// ============================================
// LOOP PRINCIPAL
// ============================================
void loop() {
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    
    if (comando.startsWith("START")) {
      // Parsear: START,fc,fm,m,offset,amp
      int idx1 = comando.indexOf(',');
      int idx2 = comando.indexOf(',', idx1 + 1);
      int idx3 = comando.indexOf(',', idx2 + 1);
      int idx4 = comando.indexOf(',', idx3 + 1);
      int idx5 = comando.indexOf(',', idx4 + 1);
      
      if (idx1 > 0 && idx2 > 0 && idx3 > 0 && idx4 > 0 && idx5 > 0) {
        fc = comando.substring(idx1 + 1, idx2).toFloat();
        fm = comando.substring(idx2 + 1, idx3).toFloat();
        m = comando.substring(idx3 + 1, idx4).toFloat();
        offset_v = comando.substring(idx4 + 1, idx5).toFloat();
        amplitud = comando.substring(idx5 + 1).toFloat();
        
        // Validaciones
        if (fc > SAMPLE_RATE / 10) {
          Serial.println("ERROR: fc muy alta para este sample rate");
          return;
        }
        
        m = constrain(m, 0.0, 1.0);
        offset_v = constrain(offset_v, 0.0, 3.3);
        amplitud = constrain(amplitud, 0.0, 1.65);
        
        // Iniciar generación
        sample_count = 0;
        generando = true;
        timerStart(timer);
        digitalWrite(LED, HIGH);
        
        Serial.println("=================================");
        Serial.println("✓ GENERANDO SEÑAL AM");
        Serial.println("=================================");
        Serial.print("  Portadora:    "); Serial.print(fc); Serial.println(" Hz");
        Serial.print("  Moduladora:   "); Serial.print(fm); Serial.println(" Hz");
        Serial.print("  Modulación:   "); Serial.print(m * 100); Serial.println(" %");
        Serial.print("  Offset:       "); Serial.print(offset_v); Serial.println(" V");
        Serial.print("  Amplitud:     "); Serial.print(amplitud); Serial.println(" V");
        Serial.println("=================================\n");
      } else {
        Serial.println("ERROR: Formato incorrecto");
        Serial.println("Uso: START,fc,fm,m,offset,amp");
      }
      
    } else if (comando == "STOP") {
      generando = false;
      timerStop(timer);
      dacWrite(DAC_PIN, 127);  // Punto medio
      digitalWrite(LED, LOW);
      Serial.println("\n✓ Señal detenida\n");
      
    } else {
      Serial.println("Comando no reconocido");
    }
  }
  
  delay(10);
}