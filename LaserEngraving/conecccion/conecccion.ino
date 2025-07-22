#include <WiFi.h>
#include <WebServer.h>
#include <AccelStepper.h>

// --- Configuración de WiFi y Servidor Web ---
const char* ssid = "Comunidad_UNMED";
const char* password = "wifi_med_213";
WebServer server(80);

const int inputPin = 34; // Pin para el sensor analógico (si se usa)
String serialMessage = ""; // Se usa para almacenar el texto recibido de la web

// --- HTML para la interfaz web ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>CNC Drawing</title>
  <meta charset="UTF-8"> <!-- Importante para la correcta visualización de caracteres -->
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      margin: 0;
      padding: 20px;  
      background-color: #f0f0f0;
    }
    .container {
      max-width: 500px;
      margin: 0 auto;
      background-color: white;
      padding: 20px;
      border-radius: 15px;
      box-shadow: 0 4px 8px rgba(0,0,0,0.1);
    }
    h1 {
      color: #2c3e50;
      margin-bottom: 30px;
    }
    .display {
      background-color: #ecf0f1;
      padding: 20px;
      border-radius: 10px;
      margin: 20px 0;
      font-size: 24px;
      min-height: 50px;
    }
    .input-group {
      margin: 25px 0;
    }
    input[type="text"] {
      width: 70%;
      padding: 12px;
      border: 2px solid #3498db;
      border-radius: 5px;
      font-size: 16px;
    }
    button {
      background-color: #3498db;
      color: white;
      border: none;
      padding: 12px 20px;
      border-radius: 5px;
      cursor: pointer;
      font-size: 16px;
      margin-left: 10px;
      transition: background 0.3s;
    }
    button:hover {
      background-color: #2980b9;
    }
    .status {
      margin-top: 20px;
      color: #7f8c8d;
      font-style: italic;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>CNC Drawing</h1>
    
    <div class="display" id="sensorValue">--</div>
    
    <div class="input-group">
      <input type="text" id="userInput" placeholder="Escriba el texto a dibujar">
      <button onclick="sendText()">Enviar</button>
    </div>
    
    <div class="status" id="status"></div>
  </div>

  <script>
    function sendText() {
      const input = document.getElementById("userInput").value;
      if(input.trim() === "") {
        document.getElementById("status").innerText = "Por favor ingrese un texto";
        return;
      }
      
      fetch('/sendText', {
        method: 'POST',
        headers: {
          'Content-Type': 'text/plain'
        },
        body: input
      })
      .then(response => {
        if(response.ok) {
          document.getElementById("status").innerText = "Texto enviado correctamente";
          document.getElementById("userInput").value = "";
          setTimeout(() => {
            document.getElementById("status").innerText = "";
          }, 2000);
        }
      })
      .catch(error => {
        document.getElementById("status").innerText = "Error al enviar: " + error;
      });
    }

    function updateSensor() {
      fetch('/getSensor')
        .then(response => response.text())
        .then(data => {
          document.getElementById("sensorValue").innerText = data;
        });
    }

    setInterval(updateSensor, 500);
    updateSensor(); // Llamar inmediatamente al cargar
  </script>
</body>
</html>
)rawliteral";


// --- Configuración de Control CNC ---
#define STEP1 2
#define DIR1 4
#define STEP2 12
#define DIR2 14

// PWM manual para láser (o levantamiento de lápiz/herramienta)
const int laserPin = 27;
const int frecuencia = 5000;      // 5 kHz
const float dutyCycle = 0.5;      // 50%
const unsigned long periodo = 200;  // 200 μs (1/5000Hz)

// Crear objetos de motor
AccelStepper motorY(AccelStepper::DRIVER, STEP1, DIR1);  // Eje Y
AccelStepper motorX(AccelStepper::DRIVER, STEP2, DIR2);  // Eje X

// ====================================================================
// --- PARÁMETROS CRÍTICOS DE CALIBRACIÓN Y RENDIMIENTO ---
// AJUSTA ESTOS VALORES SEGÚN TU MÁQUINA CNC REAL
// ====================================================================

// STEPS_PER_MM: Pasos del motor necesarios para mover 1 milímetro.
// ESTE ES EL PARÁMETRO MÁS IMPORTANTE PARA LA PRECISIÓN DEL DIBUJO.
// Si tu motor es de 200 pasos/revolución y tu correa tiene un paso de 2mm/revolución,
// y usas microstepping de 1/8, entonces 200 * 8 / 2 = 800 STEPS_PER_MM
// DEBES CALIBRAR ESTO MIDiendo MOVIMIENTOS REALES.
const int STEPS_PER_MM = 80; // Valor inicial, ¡AJUSTA ESTO!

// MAX_SPEED: Velocidad máxima a la que los motores pueden moverse (pasos/segundo).
// Si es demasiado alta, los motores pueden saltar pasos o vibrar.
// Empieza con un valor bajo y auméntalo gradualmente.
const float MAX_SPEED = 500.0; // Valor anterior a la duplicación

// ACCELERATION: Aceleración/desaceleración de los motores (pasos/segundo^2).
// Si es demasiado alta, los motores pueden saltar pasos al arrancar/parar.
// Empieza con un valor bajo y auméntalo gradualmente.
const float ACCELERATION = 200.0; // Valor anterior a la duplicación

// MIN_MOVEMENT_TIME: Tiempo mínimo en milisegundos que un movimiento debe durar.
// Esto es para asegurar que el movimiento físico se complete antes de enviar "OK".
// Ajusta este valor si los motores parecen detenerse antes de terminar.
const unsigned long MIN_MOVEMENT_TIME = 500; // 500 ms (0.5 segundos) por defecto, ajusta según necesidad.

// ====================================================================

// Variables de estado PWM
unsigned long prevMicros = 0;
bool laserState = LOW;
bool laserEnabled = false;

// Bandera para indicar que un comando ha sido procesado y se espera que el movimiento termine
bool commandInProgress = false;
unsigned long commandStartTime = 0; // Para medir el tiempo de ejecución del comando

// --- Prototipos de Funciones ---
void handleLaserPWM();
void processSerialCommand(String input); // Declaración de la función con el argumento


void setup() {
  Serial.begin(115200);
  pinMode(inputPin, INPUT); // Para el sensor analógico
  
  // --- Configuración de WiFi ---
  Serial.println("\nIntentando conectar a WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado! IP: ");
  Serial.println(WiFi.localIP()); // Imprime la IP una vez después de la conexión
  Serial.println("Servidor HTTP iniciado"); // Este es el segundo mensaje de inicio que Python ignorará

  // --- Rutas del Servidor Web ---
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", index_html);
  });

  server.on("/getSensor", HTTP_GET, []() {
    int sensorValue = analogRead(inputPin);
    server.send(200, "text/plain", String(sensorValue));
  });

  server.on("/sendText", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      serialMessage = server.arg("plain");
      Serial.println("Texto recibido: " + serialMessage);
      Serial.print("DRAW:"); // Comando a enviar a un posible script Python
      Serial.println(serialMessage);
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Texto no recibido");
    }
  });

  server.begin();

  // --- Configuración de Motores CNC ---
  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, LOW); // Asegurarse de que el láser esté apagado al inicio

  motorX.setMaxSpeed(MAX_SPEED);
  motorX.setAcceleration(ACCELERATION);
  motorY.setMaxSpeed(MAX_SPEED);
  motorY.setAcceleration(ACCELERATION);

  motorX.setCurrentPosition(0); // Establece la posición inicial de los motores a 0
  motorY.setCurrentPosition(0); // Esto es fundamental para un punto de partida consistente.
                                // Si tienes finales de carrera, aquí iría una rutina de homing.
}

void loop() {
  server.handleClient();
  
  // --- Manejar PWM del Láser ---
  handleLaserPWM();
  
  // --- Manejar Movimiento de Motores ---
  // AccelStepper necesita ser llamado continuamente para mover los motores.
  motorX.run(); 
  motorY.run();

  // Verificar si los motores han llegado a su destino
  bool motorsAtTarget = (motorX.distanceToGo() == 0 && motorY.distanceToGo() == 0);

  // --- Procesar Comandos Seriales (para control CNC) ---
  // Siempre leer el puerto serial para evitar desbordamientos de buffer.
  if (Serial.available()) {
    // Leer el comando entrante
    String incomingCommand = Serial.readStringUntil('\n');
    incomingCommand.trim();

    // Si no hay un comando en progreso O si el comando en progreso ya ha terminado su movimiento físico
    // (es decir, motorsAtTarget Y ha pasado el tiempo mínimo de movimiento)
    if (!commandInProgress || (motorsAtTarget && (millis() - commandStartTime >= MIN_MOVEMENT_TIME))) {
      // Si hay un comando en progreso que ya terminó, enviamos el OK antes de procesar el nuevo
      if (commandInProgress && motorsAtTarget && (millis() - commandStartTime >= MIN_MOVEMENT_TIME)) { 
          Serial.println("OK"); // Enviar confirmación a Python
          commandInProgress = false; // Resetear la bandera
          Serial.println("DEBUG: Movimiento completado. OK enviado."); // Mensaje de depuración
      }
      // Procesar el nuevo comando
      processSerialCommand(incomingCommand); // Pasa el comando leído a la función
    } else {
      // Si hay un comando en progreso y no ha terminado, ignorar el comando entrante por ahora
      // Esto evita que Python envíe comandos demasiado rápido.
      Serial.print("DEBUG: Comando ignorado (movimiento en curso): "); // Añadido DEBUG
      Serial.println(incomingCommand);
    }
  }

  // --- Enviar confirmación "OK" a Python solo cuando el movimiento ha terminado y ha pasado un tiempo mínimo ---
  // Esta lógica se mantiene para asegurar que el OK se envíe incluso si no hay nuevos comandos entrantes.
  if (commandInProgress && motorsAtTarget && (millis() - commandStartTime >= MIN_MOVEMENT_TIME)) { 
    Serial.println("OK"); // Enviar confirmación a Python
    commandInProgress = false; // Resetear la bandera
    Serial.println("DEBUG: Movimiento completado. OK enviado."); // Mensaje de depuración
  }
  
  delay(10); // Pequeño retraso para permitir la ejecución de otras tareas
}

// --- Funciones de Ayuda CNC ---

void handleLaserPWM() {
  // DEBUG: Imprime el estado de laserEnabled en cada ciclo
  // Serial.print("DEBUG: handleLaserPWM - laserEnabled: ");
  // Serial.println(laserEnabled ? "TRUE" : "FALSE");

  if (!laserEnabled) {
    if (digitalRead(laserPin) == HIGH) { // Solo si estaba HIGH, para evitar spam
        digitalWrite(laserPin, LOW);
        Serial.println("DEBUG: Laser apagado por !laserEnabled."); // DEBUG
    }
    return;
  }
  
  unsigned long currentMicros = micros();
  unsigned long elapsed = currentMicros - prevMicros;
  
  if (laserState == HIGH && elapsed >= periodo * dutyCycle) {
    digitalWrite(laserPin, LOW);
    laserState = LOW;
    Serial.println("DEBUG: Laser LOW (ciclo PWM)."); // DEBUG
    prevMicros = currentMicros;
  }
  else if (laserState == LOW && elapsed >= periodo * (1 - dutyCycle)) {
    digitalWrite(laserPin, HIGH);
    laserState = HIGH;
    Serial.println("DEBUG: Laser HIGH (ciclo PWM)."); // DEBUG
    prevMicros = currentMicros;
  }
}

// Modificada para aceptar el comando como argumento
void processSerialCommand(String input) {
  // input.trim(); // Ya se hizo el trim al leer en loop()

  if (input == "L") {  // Levantar herramienta (ej. lápiz o láser apagado)
    laserEnabled = false;
    digitalWrite(laserPin, LOW); // Asegurarse de apagar el láser inmediatamente
    motorY.move(-5 * STEPS_PER_MM); // Movimiento de ejemplo para levantar (ajusta si es necesario para tu herramienta)
    Serial.print("DEBUG: L - Posición actual X:"); Serial.print(motorX.currentPosition()); Serial.print(", Y:"); Serial.println(motorY.currentPosition());
    Serial.print("DEBUG: L - Moviendo Y a: "); Serial.println(motorY.targetPosition());
    Serial.println("Comando 'L' recibido: Herramienta levantada. (laserEnabled = FALSE)"); // Mensaje de depuración
    commandInProgress = true; // Establecer bandera para esperar finalización del movimiento
    commandStartTime = millis(); // Registrar tiempo de inicio del comando
  }
  else {
    int commaIndex = input.indexOf(',');
    if (commaIndex > 0) {
      long x = input.substring(0, commaIndex).toInt();
      long y = input.substring(commaIndex + 1).toInt();
      
      Serial.print("DEBUG: Movimiento - Posición actual X:"); Serial.print(motorX.currentPosition()); Serial.print(", Y:"); Serial.println(motorY.currentPosition());
      motorX.moveTo(x); 
      motorY.moveTo(y);
      Serial.print("DEBUG: Movimiento - Objetivo X:"); Serial.print(motorX.targetPosition()); Serial.print(", Y:"); Serial.println(motorY.targetPosition());
      
      // Preparar PWM para el siguiente ciclo (herramienta habilitada/encendida para dibujar)
      laserEnabled = true;
      laserState = LOW; // Asegurarse de que el ciclo PWM se reinicie con HIGH
      prevMicros = micros(); // Reiniciar el contador de tiempo para el PWM del láser
      Serial.print("Comando de movimiento recibido: Moviendo a X:");
      Serial.print(x);
      Serial.print(", Y:");
      Serial.println(y);
      Serial.println("DEBUG: laserEnabled = TRUE para movimiento."); // DEBUG
      commandInProgress = true; // Establecer bandera para esperar finalización del movimiento
      commandStartTime = millis(); // Registrar tiempo de inicio del comando
    } else {
      Serial.print("DEBUG: Comando serial desconocido: "); // Añadido DEBUG
      Serial.println(input);
    }
  }
}

