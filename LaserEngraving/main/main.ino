#include <WiFi.h>
#include <WebServer.h>
#include <AccelStepper.h>
#include <vector>

// --- Configuración WiFi ---
const char* ssid = "Comunidad_UNMED";
const char* password = "wifi_med_213";
WebServer server(80);

// --- Configuración CNC ---
#define STEP1 2
#define DIR1 4
#define STEP2 12
#define DIR2 26
const int laserPin = 27;

// Parámetros PWM manual
const int frecuencia = 5000;      // 5 kHz
const float dutyCycle = 0.5;      // 50%
const unsigned long periodo = 200;  // 200 μs (1/5000Hz = 200μs)

// --- Parámetros calibración ---
const int STEPS_PER_MM = 80;              // Ajustar según hardware
const float MAX_SPEED = 500.0;            // Velocidad máxima (pasos/seg)
const float ACCELERATION = 200.0;         // Aceleración (pasos/seg²)
const long CHAR_WIDTH_MM = 2;            // Ancho de cada carácter (mm)
const long CHAR_HEIGHT_MM = 3;           // Altura de cada carácter (mm)
const long SPACE_BETWEEN_CHARS_MM = 2;    // Espacio entre caracteres (mm)

// --- Cálculos en pasos ---
const long MAX_X = CHAR_WIDTH_MM * STEPS_PER_MM;
const long MAX_Y = CHAR_HEIGHT_MM * STEPS_PER_MM;
const long SPACE_BETWEEN_CHARS = SPACE_BETWEEN_CHARS_MM * STEPS_PER_MM;

// --- Objetos de motor ---
AccelStepper motorY(AccelStepper::DRIVER, STEP1, DIR1);  // Eje Y
AccelStepper motorX(AccelStepper::DRIVER, STEP2, DIR2);  // Eje X

// --- Estructura para coordenadas ---
struct Point {
    long x;
    long y;
};

// --- Variables de control de dibujo ---
String textToDraw = "";
int currentCharIndex = -1;
std::vector<Point> currentCharPath;
int currentPointIndex = 0;
long xOffset = 0;
bool drawingInProgress = false;
bool repositioningMove = true;

// Variables para PWM manual
unsigned long prevMicros = 0;
bool laserState = LOW;
bool laserEnabled = false;

// --- HTML para interfaz web ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>CNC Drawing</title>
  <meta charset="UTF-8">
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
    .cnc-status {
      margin-top: 15px;
      padding: 10px;
      background-color: #e8f4f8;
      border-radius: 5px;
      font-weight: bold;
    }
    .laser-status {
      margin-top: 10px;
      padding: 8px;
      border-radius: 5px;
      font-weight: bold;
    }
    .laser-on {
      background-color: #ff5252;
      color: white;
    }
    .laser-off {
      background-color: #e0e0e0;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>CNC Laser Drawing</h1>
    
    <div class="cnc-status" id="cncStatus">Listo</div>
    <div class="laser-status laser-off" id="laserStatus">LÁSER: APAGADO</div>
    
    <div class="input-group">
      <input type="text" id="userInput" placeholder="Escriba texto (MAYÚSCULAS)" maxlength="20">
      <button onclick="sendText()">Dibujar</button>
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
      
      document.getElementById("cncStatus").innerText = "Procesando...";
      document.getElementById("status").innerText = "";
      
      fetch('/sendText', {
        method: 'POST',
        headers: {'Content-Type': 'text/plain'},
        body: input.toUpperCase()
      })
      .then(response => {
        if(response.ok) {
          document.getElementById("status").innerText = "Texto enviado! Dibujando...";
          document.getElementById("userInput").value = "";
        } else {
          document.getElementById("status").innerText = "Error al enviar";
        }
      })
      .catch(error => {
        document.getElementById("status").innerText = "Error: " + error;
      });
    }

    function updateStatus() {
      fetch('/getCNCStatus')
        .then(response => response.text())
        .then(data => {
          document.getElementById("cncStatus").innerText = data;
        });
        
      fetch('/getLaserStatus')
        .then(response => response.text())
        .then(data => {
          const laserStatus = document.getElementById("laserStatus");
          if(data === "ON") {
            laserStatus.className = "laser-status laser-on";
            laserStatus.innerText = "LÁSER: ENCENDIDO";
          } else {
            laserStatus.className = "laser-status laser-off";
            laserStatus.innerText = "LÁSER: APAGADO";
          }
        });
    }

    setInterval(updateStatus, 300);
  </script>
</body>
</html>
)rawliteral";

// --- Generación de coordenadas para caracteres ---
std::vector<Point> getCharCoordinates(char c, int xOffset = 0) {
    std::vector<Point> coords;
    c = toupper(c);

    auto move = [&](int x, int y) {
        coords.push_back({x + xOffset, y});
    };

    auto lift = [&]() {
        coords.push_back({-1, -1});
    };

    switch (c) {
        case 'A':
            move(0, 0); move(MAX_X / 2, MAX_Y); lift();
            move(MAX_X, 0); move(MAX_X / 2, MAX_Y); lift();
            move(MAX_X / 4, MAX_Y / 2); move(MAX_X * 3 / 4, MAX_Y / 2);
            break;
        case 'B':
            move(0, 0); move(0, MAX_Y); lift();
            move(0, MAX_Y); move(MAX_X * 3 / 4, MAX_Y); move(MAX_X, MAX_Y * 3 / 4); move(MAX_X * 3 / 4, MAX_Y / 2); move(0, MAX_Y / 2); lift();
            move(0, MAX_Y / 2); move(MAX_X * 3 / 4, MAX_Y / 2); move(MAX_X, MAX_Y / 4); move(MAX_X * 3 / 4, 0); move(0, 0);
            break;
        case 'C':
            move(MAX_X, MAX_Y); move(0, MAX_Y); move(0, 0); move(MAX_X, 0);
            break;
        case 'D':
            move(0, 0); move(0, MAX_Y); lift();
            move(0, MAX_Y); move(MAX_X, MAX_Y * 3 / 4); move(MAX_X, MAX_Y / 4); move(0, 0);
            break;
        case 'E':
            move(0, 0); move(0, MAX_Y); lift();
            move(0, MAX_Y); move(MAX_X, MAX_Y); lift();
            move(0, MAX_Y / 2); move(MAX_X * 3 / 4, MAX_Y / 2); lift();
            move(0, 0); move(MAX_X, 0);
            break;
        case 'F':
            move(0, 0); move(0, MAX_Y); lift();
            move(0, MAX_Y); move(MAX_X, MAX_Y); lift();
            move(0, MAX_Y / 2); move(MAX_X * 3 / 4, MAX_Y / 2);
            break;
        case 'G':
            move(MAX_X, MAX_Y); move(0, MAX_Y); move(0, 0); move(MAX_X, 0); move(MAX_X, MAX_Y / 2); move(MAX_X / 2, MAX_Y / 2);
            break;
        case 'H':
            move(0, 0); move(0, MAX_Y); lift();
            move(MAX_X, 0); move(MAX_X, MAX_Y); lift();
            move(0, MAX_Y / 2); move(MAX_X, MAX_Y / 2);
            break;
        case 'I':
            move(MAX_X / 2, 0); move(MAX_X / 2, MAX_Y); lift();
            move(0, MAX_Y); move(MAX_X, MAX_Y); lift();
            move(0, 0); move(MAX_X, 0);
            break;
        case 'J':
            move(MAX_X, MAX_Y); move(MAX_X, 0); move(MAX_X / 2, 0); move(0, MAX_Y / 4);
            break;
        case 'K':
            move(0, 0); move(0, MAX_Y); lift();
            move(0, MAX_Y / 2); move(MAX_X, MAX_Y); lift();
            move(0, MAX_Y / 2); move(MAX_X, 0);
            break;
        case 'L':
            move(0, MAX_Y); move(0, 0); lift();
            move(0, 0); move(MAX_X, 0);
            break;
        case 'M':
            move(0, 0); move(0, MAX_Y); lift();
            move(0, MAX_Y); move(MAX_X / 2, MAX_Y / 2); lift();
            move(MAX_X / 2, MAX_Y / 2); move(MAX_X, MAX_Y); lift();
            move(MAX_X, MAX_Y); move(MAX_X, 0);
            break;
        case 'N':
            move(0, 0); move(0, MAX_Y); lift();
            move(0, MAX_Y); move(MAX_X, 0); lift();
            move(MAX_X, 0); move(MAX_X, MAX_Y);
            break;
        case 'O':
            move(0, 0); move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, 0); move(0, 0);
            break;
        case 'P':
            move(0, 0); move(0, MAX_Y); lift();
            move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, MAX_Y / 2); move(0, MAX_Y / 2);
            break;
        case 'Q':
            move(0, 0); move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, 0); move(0, 0); lift();
            move(MAX_X / 2, MAX_Y / 2); move(MAX_X, 0);
            break;
        case 'R':
            move(0, 0); move(0, MAX_Y); lift();
            move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, MAX_Y / 2); move(0, MAX_Y / 2); lift();
            move(MAX_X / 2, MAX_Y / 2); move(MAX_X, 0);
            break;
        case 'S':
            move(MAX_X, MAX_Y); move(0, MAX_Y); move(0, MAX_Y / 2); move(MAX_X, MAX_Y / 2); move(MAX_X, 0); move(0, 0);
            break;
        case 'T':
            move(0, MAX_Y); move(MAX_X, MAX_Y); lift();
            move(MAX_X / 2, MAX_Y); move(MAX_X / 2, 0);
            break;
        case 'U':
            move(0, MAX_Y); move(0, 0); move(MAX_X, 0); move(MAX_X, MAX_Y);
            break;
        case 'V':
            move(0, MAX_Y); move(MAX_X / 2, 0); move(MAX_X, MAX_Y);
            break;
        case 'W':
            move(0, MAX_Y); move(0, 0); move(MAX_X / 2, MAX_Y / 2); move(MAX_X, 0); move(MAX_X, MAX_Y);
            break;
        case 'X':
            move(0, MAX_Y); move(MAX_X, 0); lift();
            move(MAX_X, MAX_Y); move(0, 0);
            break;
        case 'Y':
            move(0, MAX_Y); move(MAX_X / 2, MAX_Y / 2); lift();
            move(MAX_X, MAX_Y); move(MAX_X / 2, MAX_Y / 2); lift();
            move(MAX_X / 2, MAX_Y / 2); move(MAX_X / 2, 0);
            break;
        case 'Z':
            move(0, MAX_Y); move(MAX_X, MAX_Y); move(0, 0); move(MAX_X, 0);
            break;
        case '0':
            move(0, 0); move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, 0); move(0, 0);
            break;
        case '1':
            move(MAX_X / 2, 0); move(MAX_X / 2, MAX_Y); lift();
            move(MAX_X / 4, MAX_Y * 3 / 4); move(MAX_X / 2, MAX_Y);
            break;
        case '2':
            move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, MAX_Y / 2); move(0, 0); move(MAX_X, 0);
            break;
        case '3':
            move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, MAX_Y / 2); move(0, MAX_Y / 2); lift();
            move(MAX_X, MAX_Y / 2); move(MAX_X, 0); move(0, 0);
            break;
        case '4':
            move(MAX_X, MAX_Y); move(0, MAX_Y / 2); move(MAX_X, MAX_Y / 2); lift();
            move(MAX_X, MAX_Y / 2); move(MAX_X, 0);
            break;
        case '5':
            move(MAX_X, MAX_Y); move(0, MAX_Y); move(0, MAX_Y / 2); move(MAX_X, MAX_Y / 2); move(MAX_X, 0); move(0, 0);
            break;
        case '6':
            move(MAX_X, MAX_Y); move(0, MAX_Y); move(0, 0); move(MAX_X, 0); move(MAX_X, MAX_Y / 2); move(0, MAX_Y / 2);
            break;
        case '7':
            move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, 0);
            break;
        case '8':
            move(0, MAX_Y / 2); move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, 0); move(0, 0); move(0, MAX_Y / 2); move(MAX_X, MAX_Y / 2);
            break;
        case '9':
            move(MAX_X, MAX_Y / 2); move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, 0); move(0, 0);
            break;
        case ' ': // Espacio
            lift();
            move(0, 0); move(MAX_X, 0);
            lift();
            break;
        default:
            break;
    }

    return coords;
}

// Función para manejar el PWM del láser manualmente
void handleLaserPWM() {
  if (!laserEnabled) {
    if (digitalRead(laserPin) == HIGH) {
        digitalWrite(laserPin, LOW);
        // Serial.println("DEBUG: Laser apagado por !laserEnabled.");
    }
    return;
  }
  
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

void setup() {
    Serial.begin(115200);
    pinMode(laserPin, OUTPUT);
    digitalWrite(laserPin, LOW);

    // Conexión WiFi
    WiFi.begin(ssid, password);
    Serial.print("Conectando a WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConectado! IP: " + WiFi.localIP().toString());

    // Configuración de rutas web
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", index_html);
    });
    
    server.on("/sendText", HTTP_POST, []() {
        if (server.hasArg("plain")) {
            textToDraw = server.arg("plain");
            textToDraw.toUpperCase();
            currentCharIndex = 0;
            xOffset = 0;
            drawingInProgress = true;
            repositioningMove = true;
            currentCharPath.clear();
            currentPointIndex = 0;
            laserEnabled = false; // Asegurar que el láser empiece apagado
            digitalWrite(laserPin, LOW);
            server.send(200, "text/plain", "OK");
            Serial.println("Iniciando dibujo: " + textToDraw);
        } else {
            server.send(400, "text/plain", "Error: Texto no recibido");
        }
    });
    
    server.on("/getCNCStatus", HTTP_GET, []() {
        String status = "Listo";
        if (drawingInProgress) {
            status = "Dibujando: " + String(currentCharIndex + 1) + "/" + String(textToDraw.length());
        }
        server.send(200, "text/plain", status);
    });
    
    server.on("/getLaserStatus", HTTP_GET, []() {
        server.send(200, "text/plain", laserEnabled ? "ON" : "OFF");
    });
    
    server.begin();

    // Configuración de motores
    motorX.setMaxSpeed(MAX_SPEED);
    motorX.setAcceleration(ACCELERATION);
    motorY.setMaxSpeed(MAX_SPEED);
    motorY.setAcceleration(ACCELERATION);
    motorX.setCurrentPosition(0);
    motorY.setCurrentPosition(0);
    
    Serial.println("Sistema CNC inicializado");
}

void loop() {
    server.handleClient();
    
    // Manejar el PWM del láser en cada iteración
    handleLaserPWM();
    
    if (!motorX.isRunning() && !motorY.isRunning()) {
        if (drawingInProgress) {
            processDrawing();
        }
    }
    
    motorX.run();
    motorY.run();
    delay(1); // Pequeña pausa para evitar bloqueos
}

void processDrawing() {
    if (currentCharIndex >= textToDraw.length()) {
        // Dibujo completado
        drawingInProgress = false;
        laserEnabled = false;
        digitalWrite(laserPin, LOW);
        Serial.println("Dibujo completado!");
        return;
    }

    // Cargar nuevo carácter si es necesario
    if (currentCharPath.empty()) {
        char currentChar = textToDraw[currentCharIndex];
        if (currentChar == ' ') {
            // Manejar espacio: solo desplazamiento
            xOffset += MAX_X + SPACE_BETWEEN_CHARS;
            currentCharIndex++;
            repositioningMove = true;
            laserEnabled = false;
            digitalWrite(laserPin, LOW);
            Serial.println("Espacio detectado");
            return;
        }
        
        currentCharPath = getCharCoordinates(currentChar, xOffset);
        currentPointIndex = 0;
        xOffset += MAX_X + SPACE_BETWEEN_CHARS;
        Serial.println("Procesando caracter: " + String(currentChar));
    }

    // Procesar siguiente punto
    if (currentPointIndex < currentCharPath.size()) {
        Point p = currentCharPath[currentPointIndex++];
        
        if (p.x == -1 && p.y == -1) {
            // Levantar herramienta (apagar láser)
            laserEnabled = false;
            digitalWrite(laserPin, LOW); // Apagar inmediatamente
            repositioningMove = true;
            Serial.println("Laser APAGADO (reposicionamiento)");
        } else {
            // Mover a nueva posición
            motorX.moveTo(p.x);
            motorY.moveTo(p.y);
            
            // Control láser
            if (repositioningMove) {
                // Movimiento de reposicionamiento: apagar láser
                laserEnabled = false;
                repositioningMove = false;
                Serial.println("Moviendo a posición inicial: (" + String(p.x) + ", " + String(p.y) + ")");
            } else {
                // Movimiento de dibujo: encender láser con PWM
                laserEnabled = true;
                Serial.println("Dibujando en: (" + String(p.x) + ", " + String(p.y) + ")");
            }
        }
    } else {
        // Carácter completado
        currentCharPath.clear();
        currentPointIndex = 0;
        currentCharIndex++;
        repositioningMove = true;
        laserEnabled = false; // Apagar láser al terminar carácter
        digitalWrite(laserPin, LOW);

        // Desplazar el cabezal en X para separar letras visualmente
        motorX.moveTo(xOffset);  // xOffset ya incluye el avance del siguiente carácter
        Serial.println("Espaciado entre letras");

    }
}