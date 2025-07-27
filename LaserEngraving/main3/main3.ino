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

// Pines sensores MQ135
const int sensorPin1 = 35;  // Sensor 1
const int sensorPin2 = 34;  // Sensor 2

// constantes para sensores

const long sensorInterval = 2000;  // Actualizar sensores cada 2 segundos
unsigned long lastSensorCheck = 0;
const int SAFE_PPM_LIMIT = 360;  // Límite de seguridad en ppm

// Parámetros PWM manual
const int frecuencia = 10000;    // 5 kHz
const float dutyCycle = 0.9;    // 50%
const unsigned long periodo = 200;  // 200 μs (1/5000Hz = 200μs)

// --- Parámetros calibración ---
const int STEPS_PER_MM = 80;            // Ajustar según hardware
const float MAX_SPEED = 1000.0;          // Velocidad máxima (pasos/seg)
const float ACCELERATION = 400.0;       // Aceleración (pasos/seg²)
const long CHAR_WIDTH_MM = 1;         // Ancho de cada carácter (mm)
const long CHAR_HEIGHT_MM = 1.5;        // Altura de cada carácter (mm)
const long SPACE_BETWEEN_CHARS_MM = 1;  // Espacio entre caracteres (mm)

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
bool repositioningMove = true;

// Variables para PWM manual
unsigned long prevMicros = 0;
bool laserState = LOW;
bool laserEnabled = false;


// --- Estados de la máquina de estados ---
enum CNC_State {
    IDLE,
    PROCESSING_TEXT,
    DRAWING_CHARACTER,
    PAUSED,
    STOPPED
};

CNC_State currentState = IDLE;

// Declaraciones anticipadas de funciones
std::vector<Point> getCharCoordinates(char c, int xOffset = 0);
void handleLaserPWM();
void processDrawing();
void enterState(CNC_State newState);
String getCNCStatusString();

// --- HTML para interfaz web ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>Sistema CNC Avanzado</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    :root {
      --primary: #3498db;
      --secondary: #2c3e50;
      --success: #2ecc71;
      --danger: #e74c3c;
      --warning: #f39c12;
      --dark: #34495e;
      --light: #ecf0f1;
    }
    
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }
    
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: linear-gradient(135deg, #1a2a6c, #b21f1f, #1a2a6c);
      color: #333;
      line-height: 1.6;
      padding: 20px;
      min-height: 100vh;
    }
    
    .container {
      max-width: 1200px;
      margin: 0 auto;
      display: grid;
      grid-template-columns: 1fr;
      gap: 25px;
    }
    
    .card {
      background: rgba(255, 255, 255, 0.92);
      border-radius: 15px;
      box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
      padding: 25px;
      transition: transform 0.3s ease;
    }
    
    .card:hover {
      transform: translateY(-5px);
    }
    
    header {
      text-align: center;
      margin-bottom: 30px;
      padding: 20px;
    }
    
    h1 {
      color: white;
      font-size: 2.8rem;
      text-shadow: 0 2px 4px rgba(0,0,0,0.3);
      margin-bottom: 10px;
    }
    
    .subtitle {
      color: var(--light);
      font-size: 1.2rem;
      max-width: 600px;
      margin: 0 auto;
    }
    
    h2 {
      color: var(--secondary);
      margin-bottom: 20px;
      padding-bottom: 10px;
      border-bottom: 2px solid var(--primary);
      font-size: 1.8rem;
    }
    
    .status-cards {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
      gap: 20px;
      margin-bottom: 25px;
    }
    
    .status-card {
      background: white;
      border-radius: 12px;
      padding: 20px;
      box-shadow: 0 5px 15px rgba(0,0,0,0.08);
      text-align: center;
    }
    
    .status-title {
      font-size: 1.1rem;
      color: var(--dark);
      margin-bottom: 10px;
    }
    
    .status-value {
      font-size: 1.8rem;
      font-weight: bold;
    }
    
    .cnc-status {
      color: var(--primary);
    }
    
    .laser-on {
      color: var(--danger);
      animation: pulse 1.5s infinite;
    }
    
    .laser-off {
      color: var(--dark);
    }
    
    @keyframes pulse {
      0% { opacity: 0.7; }
      50% { opacity: 1; }
      100% { opacity: 0.7; }
    }
    
    .control-panel {
      display: grid;
      grid-template-columns: 1fr;
      gap: 20px;
    }
    
    .input-group {
      display: flex;
      margin-bottom: 15px;
    }
    
    input[type="text"] {
      flex: 1;
      padding: 14px 18px;
      border: 2px solid #ddd;
      border-radius: 8px;
      font-size: 1.1rem;
      transition: border-color 0.3s;
    }
    
    input[type="text"]:focus {
      border-color: var(--primary);
      outline: none;
      box-shadow: 0 0 0 3px rgba(52, 152, 219, 0.2);
    }
    
    .btn-group {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
      gap: 12px;
      margin-top: 15px;
    }
    
    button {
      border: none;
      padding: 14px 20px;
      border-radius: 8px;
      cursor: pointer;
      font-size: 1.1rem;
      font-weight: 600;
      transition: all 0.3s ease;
      box-shadow: 0 4px 6px rgba(0,0,0,0.1);
    }
    
    button:disabled {
      opacity: 0.6;
      cursor: not-allowed;
    }
    
    .btn-primary {
      background: var(--primary);
      color: white;
    }
    
    .btn-primary:hover:not(:disabled) {
      background: #2980b9;
      transform: translateY(-2px);
    }
    
    .btn-secondary {
      background: var(--secondary);
      color: white;
    }
    
    .btn-secondary:hover:not(:disabled) {
      background: #1a252f;
      transform: translateY(-2px);
    }
    
    .btn-success {
      background: var(--success);
      color: white;
    }
    
    .btn-success:hover:not(:disabled) {
      background: #27ae60;
      transform: translateY(-2px);
    }
    
    .btn-danger {
      background: var(--danger);
      color: white;
    }
    
    .btn-danger:hover:not(:disabled) {
      background: #c0392b;
      transform: translateY(-2px);
    }
    
    .btn-warning {
      background: var(--warning);
      color: white;
    }
    
    .btn-warning:hover:not(:disabled) {
      background: #e67e22;
      transform: translateY(-2px);
    }
    
    .chart-container {
      position: relative;
      height: 250px;
      background: white;
      border-radius: 8px;
      padding: 10px;
      margin-top: 15px;
      box-shadow: 0 2px 5px rgba(0,0,0,0.05);
    }
    
    .sensor-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
      gap: 20px;
      margin-top: 20px;
    }
    
    .sensor-card {
      background: rgba(255, 255, 255, 0.95);
      border-radius: 12px;
      padding: 20px;
      box-shadow: 0 5px 15px rgba(0,0,0,0.08);
    }
    
    .sensor-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 15px;
    }
    
    .sensor-title {
      font-size: 1.2rem;
      color: #2c3e50;
      font-weight: 600;
    }
    
    .sensor-value {
      font-size: 1.8rem;
      font-weight: bold;
      color: #3498db;
    }
    
    .status-message {
      margin-top: 15px;
      padding: 12px;
      border-radius: 8px;
      text-align: center;
      font-weight: 500;
    }
    
    .status-processing {
      background: #ffeaa7;
      color: #d35400;
    }
    
    .status-ready {
      background: #d5f5e3;
      color: #27ae60;
    }
    
    .status-error {
      background: #fadbd8;
      color: #c0392b;
    }
    
    footer {
      text-align: center;
      color: white;
      margin-top: 40px;
      padding: 20px;
      font-size: 0.9rem;
    }
    
    @media (max-width: 768px) {
      .container {
        grid-template-columns: 1fr;
      }
      
      .sensor-grid {
        grid-template-columns: 1fr;
      }
      
      .input-group {
        flex-direction: column;
      }
      
      input[type="text"] {
        margin-bottom: 12px;
      }
    }
  </style>
</head>
<body>
  <header>
    <h1>Sistema CNC Avanzado</h1>
    <p class="subtitle">Control de grabado láser con monitoreo de calidad del aire</p>
  </header>
  
  <div class="container">
    <div class="card">
      <h2>Control CNC</h2>
      
      <div class="status-cards">
        <div class="status-card">
          <div class="status-title">Estado CNC</div>
          <div class="status-value cnc-status" id="cncStatus">Listo</div>
        </div>
        
        <div class="status-card">
          <div class="status-title">Estado Láser</div>
          <div class="status-value laser-off" id="laserStatus">APAGADO</div>
        </div>
      </div>
      
      <div class="control-panel">
        <div class="input-group">
          <input type="text" id="userInput" placeholder="Ingrese texto (MAYÚSCULAS)" maxlength="20">
        </div>
        
        <div class="btn-group">
          <button class="btn-primary" id="startBtn">Iniciar Grabado</button>
          <button class="btn-warning" id="pauseBtn" disabled>Pausar</button>
          <button class="btn-success" id="resumeBtn" disabled>Continuar</button>
          <button class="btn-danger" id="stopBtn" disabled>Detener</button>
        </div>
        
        <div id="statusMessage" class="status-message status-ready">Sistema listo</div>
      </div>
    </div>
    
    <div class="card">
      <h2>Monitoreo de Calidad del Aire</h2>
      
      <div class="sensor-grid">
        <div class="sensor-card">
          <div class="sensor-header">
            <div class="sensor-title">Sensor MQ135 (Pin 35)</div>
            <div class="sensor-value" id="sensorValue1">0 ppm</div>
          </div>
          <div class="chart-container">
            <canvas id="sensorChart1"></canvas>
          </div>
        </div>
        
        <div class="sensor-card">
          <div class="sensor-header">
            <div class="sensor-title">Sensor MQ135 (Pin 34)</div>
            <div class="sensor-value" id="sensorValue2">0 ppm</div>
          </div>
          <div class="chart-container">
            <canvas id="sensorChart2"></canvas>
          </div>
        </div>
      </div>
    </div>
  </div>
  
  <footer>
    <p>Sistema CNC &copy; 2023 - Universidad Médica</p>
  </footer>

  <script>
    document.addEventListener('DOMContentLoaded', function() {
      // Referencias a botones
      const startBtn = document.getElementById('startBtn');
      const pauseBtn = document.getElementById('pauseBtn');
      const resumeBtn = document.getElementById('resumeBtn');
      const stopBtn = document.getElementById('stopBtn');
      
      // Inicialización de gráficas
      const ctx1 = document.getElementById('sensorChart1').getContext('2d');
      const ctx2 = document.getElementById('sensorChart2').getContext('2d');
      
      const maxDataPoints = 20;
      const chart1 = new Chart(ctx1, {
        type: 'line',
        data: {
          labels: [],
          datasets: [{
            label: 'CO2 (ppm) - Sensor 1',
            data: [],
            borderColor: '#3498db',
            backgroundColor: 'rgba(52, 152, 219, 0.1)',
            borderWidth: 3,
            fill: true,
            tension: 0.3
          }]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          scales: {
            y: { 
              beginAtZero: true,
              grid: { 
                color: 'rgba(0,0,0,0.05)'
              }
            },
            x: { 
              grid: { 
                display: false 
              }
            }
          }
        }
      });
      
      const chart2 = new Chart(ctx2, {
        type: 'line',
        data: {
          labels: [],
          datasets: [{
            label: 'CO2 (ppm) - Sensor 2',
            data: [],
            borderColor: '#e74c3c',
            backgroundColor: 'rgba(231, 76, 60, 0.1)',
            borderWidth: 3,
            fill: true,
            tension: 0.3
          }]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          scales: {
            y: { 
              beginAtZero: true,
              grid: { 
                color: 'rgba(0,0,0,0.05)'
              }
            },
            x: { 
              grid: { 
                display: false 
              }
            }
          }
        }
      });
      
      function updateCharts(sensorData) {
        const now = new Date();
        const timeString = now.getHours() + ':' + 
                          String(now.getMinutes()).padStart(2, '0') + ':' + 
                          String(now.getSeconds()).padStart(2, '0');
        
        chart1.data.labels.push(timeString);
        chart1.data.datasets[0].data.push(sensorData.sensor1);
        
        chart2.data.labels.push(timeString);
        chart2.data.datasets[0].data.push(sensorData.sensor2);
        
        if (chart1.data.labels.length > maxDataPoints) {
          chart1.data.labels.shift();
          chart1.data.datasets[0].data.shift();
        }
        
        if (chart2.data.labels.length > maxDataPoints) {
          chart2.data.labels.shift();
          chart2.data.datasets[0].data.shift();
        }
        
        chart1.update();
        chart2.update();
        
        document.getElementById('sensorValue1').textContent = sensorData.sensor1 + ' ppm';
        document.getElementById('sensorValue2').textContent = sensorData.sensor2 + ' ppm';
      }
      
      function fetchSensorData() {
        fetch('/sensorData')
          .then(response => response.json())
          .then(data => updateCharts(data))
          .catch(error => console.error('Error obteniendo datos de sensores:', error));
      }
      
      setInterval(fetchSensorData, 2000);
      
      // Funciones de control CNC
      function sendText() {
        const input = document.getElementById('userInput').value;
        if (input.trim() === '') {
          updateStatusMessage('Por favor ingrese un texto', 'error');
          return;
        }
        
        updateStatusMessage('Enviando texto...', 'processing');
        
        fetch('/sendText', {
          method: 'POST',
          headers: {'Content-Type': 'text/plain'},
          body: input.toUpperCase()
        })
        .then(response => {
          if (response.ok) {
            updateStatusMessage('Texto enviado. Iniciando grabado...', 'processing');
            document.getElementById('userInput').value = '';
          } else {
            updateStatusMessage('Error al enviar texto', 'error');
          }
        })
        .catch(error => {
          updateStatusMessage('Error: ' + error, 'error');
        });
      }
      
      function pauseDrawing() {
        fetch('/pause')
          .then(response => {
            if (response.ok) {
              updateStatusMessage('Grabado en pausa', 'processing');
            }
          });
      }
      
      function resumeDrawing() {
        fetch('/resume')
          .then(response => {
            if (response.ok) {
              updateStatusMessage('Reanudando grabado...', 'processing');
            }
          });
      }
      
      function stopDrawing() {
        fetch('/stop')
          .then(response => {
            if (response.ok) {
              updateStatusMessage('Grabado detenido', 'ready');
            }
          });
      }
      
      // Asignar eventos a los botones
      startBtn.addEventListener('click', sendText);
      pauseBtn.addEventListener('click', pauseDrawing);
      resumeBtn.addEventListener('click', resumeDrawing);
      stopBtn.addEventListener('click', stopDrawing);
      
      let lastCncStatus = "";
      let lastLaserStatus = "";

      function updateStatus() {
        fetch('/getCNCStatus')
          .then(response => response.text())
          .then(data => {
            if (data !== lastCncStatus) {
              document.getElementById('cncStatus').textContent = data;
              lastCncStatus = data;
            }
            updateButtonStates();
          });
          
        fetch('/getLaserStatus')
          .then(response => response.text())
          .then(data => {
            if (data !== lastLaserStatus) {
              const laserStatus = document.getElementById('laserStatus');
              if(data === 'ON') {
                laserStatus.className = 'status-value laser-on';
                laserStatus.textContent = 'ENCENDIDO';
              } else {
                laserStatus.className = 'status-value laser-off';
                laserStatus.textContent = 'APAGADO';
              }
              lastLaserStatus = data;
            }
          });
      }
      
      function updateStatusMessage(message, type) {
        const statusEl = document.getElementById('statusMessage');
        statusEl.textContent = message;
        
        // Reset classes
        statusEl.className = 'status-message';
        
        // Add type-specific class
        if (type === 'processing') {
          statusEl.classList.add('status-processing');
        } else if (type === 'ready') {
          statusEl.classList.add('status-ready');
        } else if (type === 'error') {
          statusEl.classList.add('status-error');
        }
      }

      function updateButtonStates() {
        fetch('/getCNCStatus')
          .then(response => response.text())
          .then(status => {
            const startBtn = document.getElementById('startBtn');
            const pauseBtn = document.getElementById('pauseBtn');
            const resumeBtn = document.getElementById('resumeBtn');
            const stopBtn = document.getElementById('stopBtn');

            if (status.includes("Listo") || status.includes("Detenido")) {
              startBtn.disabled = false;
              pauseBtn.disabled = true;
              resumeBtn.disabled = true;
              stopBtn.disabled = true;
            } else if (status.includes("Procesando") || status.includes("Dibujando")) {
              startBtn.disabled = true;
              pauseBtn.disabled = false;
              resumeBtn.disabled = true;
              stopBtn.disabled = false;
            } else if (status.includes("PAUSADO")) {
              startBtn.disabled = true;
              pauseBtn.disabled = true;
              resumeBtn.disabled = false;
              stopBtn.disabled = false;
            }
          });
      }
      
      // Actualizar estado cada 300ms
      setInterval(updateStatus, 300);
      
      // Inicializar botones al cargar la página
      updateButtonStates();
    });
  </script>
</body>
</html>
)rawliteral";

// --- Generación de coordenadas para caracteres ---
std::vector<Point> getCharCoordinates(char c, int xOffset) {
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
        case ' ':
            lift();
            move(0, 0); move(MAX_X, 0);
            lift();
            break;
        default:
            break;
    }
    return coords;
}

// Función para manejar el PWM del láser manualmente (sin cambios)
// Función para manejar el PWM del láser manualmente
void handleLaserPWM() {
    if (!laserEnabled) {
        if (digitalRead(laserPin) == HIGH) {
            digitalWrite(laserPin, LOW);
        }
        return;
    }
    
    unsigned long currentMicros = micros();
    unsigned long elapsed = currentMicros - prevMicros;
    
    if (laserState == HIGH && elapsed >= (unsigned long)(periodo * dutyCycle)) {
        digitalWrite(laserPin, LOW);
        laserState = LOW;
        prevMicros = currentMicros;
    }
    else if (laserState == LOW && elapsed >= (unsigned long)(periodo * (1 - dutyCycle))) {
        digitalWrite(laserPin, HIGH);
        laserState = HIGH;
        prevMicros = currentMicros;
    }
}

// Función para transición entre estados
void enterState(CNC_State newState) {
    Serial.print("Cambiando estado a: ");
    switch (newState) {
        case IDLE: Serial.println("IDLE"); break;
        case PROCESSING_TEXT: Serial.println("PROCESSING_TEXT"); break;
        case DRAWING_CHARACTER: Serial.println("DRAWING_CHARACTER"); break;
        case PAUSED: Serial.println("PAUSED"); break;
        case STOPPED: Serial.println("STOPPED"); break;
    }
    currentState = newState;
    switch (currentState) {
        case IDLE:
            laserEnabled = false;
            digitalWrite(laserPin, LOW);
            motorX.stop();
            motorY.stop();
            break;
        case PROCESSING_TEXT:
            break;
        case DRAWING_CHARACTER:
            break;
        case PAUSED:
            laserEnabled = false;
            digitalWrite(laserPin, LOW);
            motorX.stop();
            motorY.stop();
            break;
        case STOPPED:
            textToDraw = "";
            currentCharIndex = -1;
            currentCharPath.clear();
            currentPointIndex = 0;
            xOffset = 0;
            repositioningMove = true;
            laserEnabled = false;
            digitalWrite(laserPin, LOW);
            motorX.stop();
            motorY.stop();
            motorX.setCurrentPosition(0);
            motorY.setCurrentPosition(0);
            break;
    }
}

String getCNCStatusString() {
    switch (currentState) {
        case IDLE: return "Listo";
        case PROCESSING_TEXT: return "Procesando...";
        case DRAWING_CHARACTER:
            if (currentCharIndex != -1 && currentCharIndex < textToDraw.length()) {
                return "Dibujando: " + String(textToDraw[currentCharIndex]) + " (" + 
                       String(currentCharIndex + 1) + "/" + String(textToDraw.length()) + ")";
            }
            return "Dibujando...";
        case PAUSED: return "PAUSADO";
        case STOPPED: return "Detenido";
        default: return "Desconocido";
    }
}


// Detener el sistema (llamado cuando se supera el límite de ppm)
void stopSystem() {
    if (currentState != IDLE && currentState != PAUSED) {
        Serial.println("¡Calidad de aire crítica! Pausando sistema...");
        enterState(PAUSED);
    }
}

void checkAirQuality() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastSensorCheck >= sensorInterval) {
        lastSensorCheck = currentMillis;
        
        int sensorValue = analogRead(sensorPin1);
        int ppm = map(sensorValue, 0, 4095, 0, 1000);
        
        Serial.print("Sensor1: ");
        Serial.print(ppm);
        Serial.println(" ppm");
        
        if (ppm >= SAFE_PPM_LIMIT) {
            stopSystem();
        }
    }
}


void setup() {
    Serial.begin(115200);
    pinMode(laserPin, OUTPUT);
    digitalWrite(laserPin, LOW);
    
    pinMode(sensorPin1, INPUT);
    pinMode(sensorPin2, INPUT);

    WiFi.begin(ssid, password);
    Serial.print("Conectando a WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConectado! IP: " + WiFi.localIP().toString());

    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", index_html);
    });
    
    server.on("/sendText", HTTP_POST, []() {
        if (server.hasArg("plain")) {
            if (currentState == IDLE || currentState == STOPPED) {
                textToDraw = server.arg("plain");
                textToDraw.toUpperCase();
                currentCharIndex = 0;
                xOffset = 0;
                currentCharPath.clear();
                currentPointIndex = 0;
                repositioningMove = true;
                enterState(PROCESSING_TEXT);
                Serial.println("Texto recibido: " + textToDraw);
                server.send(200, "text/plain", "OK");
            } else {
                server.send(409, "text/plain", "Error: Sistema ocupado");
            }
        } else {
            server.send(400, "text/plain", "Error: Texto no recibido");
        }
    });
    
    server.on("/getCNCStatus", HTTP_GET, []() {
        server.send(200, "text/plain", getCNCStatusString());
    });
    
    server.on("/getLaserStatus", HTTP_GET, []() {
        server.send(200, "text/plain", laserEnabled ? "ON" : "OFF");
    });
    
    server.on("/pause", HTTP_GET, []() {
        if (currentState == PROCESSING_TEXT || currentState == DRAWING_CHARACTER) {
            enterState(PAUSED);
            server.send(200, "text/plain", "PAUSED");
        } else {
            server.send(409, "text/plain", "Error: No se puede pausar");
        }
    });
    
    server.on("/resume", HTTP_GET, []() {
        if (currentState == PAUSED) {
            enterState(PROCESSING_TEXT);
            server.send(200, "text/plain", "RESUMED");
        } else {
            server.send(409, "text/plain", "Error: No se puede reanudar");
        }
    });
    
    server.on("/stop", HTTP_GET, []() {
        if (currentState != IDLE && currentState != STOPPED) {
            enterState(STOPPED);
            server.send(200, "text/plain", "STOPPED");
        } else {
            server.send(409, "text/plain", "Error: Ya detenido");
        }
    });
    
    server.on("/sensorData", HTTP_GET, []() {
        int sensorValue1 = analogRead(sensorPin1);
        int sensorValue2 = analogRead(sensorPin2);
        int ppm1 = map(sensorValue1, 0, 4095, 0, 1000);
        int ppm2 = map(sensorValue2, 0, 4095, 0, 1000);
        String response = "{\"sensor1\":" + String(ppm1) + ",\"sensor2\":" + String(ppm2) + "}";
        server.send(200, "application/json", response);
    });
    
    server.begin();

    motorX.setMaxSpeed(MAX_SPEED);
    motorX.setAcceleration(ACCELERATION);
    motorY.setMaxSpeed(MAX_SPEED);
    motorY.setAcceleration(ACCELERATION);
    motorX.setCurrentPosition(0);
    motorY.setCurrentPosition(0);
    
    enterState(IDLE);
}

void loop() {
    server.handleClient();
    handleLaserPWM();
    checkAirQuality();
    
    switch (currentState) {
        case IDLE:
        case STOPPED:
            break;
        case PAUSED:
            break;
        case PROCESSING_TEXT:
        case DRAWING_CHARACTER:
            if (!motorX.isRunning() && !motorY.isRunning()) {
                processDrawing();
            }
            break;
    }
    
    motorX.run();
    motorY.run();
}

// CORRECCIÓN PRINCIPAL: Función de procesamiento de dibujo modificada
void processDrawing() {
    // Verificar si se completó todo el texto
    if (currentCharIndex >= textToDraw.length()) {
        Serial.println("Dibujo completado");
        enterState(IDLE);
        return;
    }

    // Cargar nuevo carácter si es necesario
    if (currentCharPath.empty()) {
        char currentChar = textToDraw[currentCharIndex];
        Serial.println("Cargando caracter: " + String(currentChar));
        
        // Manejar espacio como carácter especial
        if (currentChar == ' ') {
            xOffset += MAX_X + SPACE_BETWEEN_CHARS;
            currentCharIndex++;
            
            // Si hay más caracteres, continuar procesando
            if (currentCharIndex < textToDraw.length()) {
                repositioningMove = true;
                laserEnabled = false;
                motorX.moveTo(xOffset);
                motorY.moveTo(0);
            }
            return;
        }
        
        // Cargar coordenadas del carácter
        currentCharPath = getCharCoordinates(currentChar, xOffset);
        currentPointIndex = 0;
        xOffset += MAX_X + SPACE_BETWEEN_CHARS;
        repositioningMove = true;
        laserEnabled = false;
        enterState(DRAWING_CHARACTER);
    }

    // Procesar puntos del carácter actual
    if (currentPointIndex < currentCharPath.size()) {
        Point p = currentCharPath[currentPointIndex];
        
        // Punto de levantamiento (desactivar láser)
        if (p.x == -1 && p.y == -1) {
            laserEnabled = false;
            repositioningMove = true;
            currentPointIndex++;
            return;
        }

        // Mover a la posición
        motorX.moveTo(p.x);
        motorY.moveTo(p.y);
        
        // Control del láser
        if (repositioningMove) {
            laserEnabled = false;
            repositioningMove = false;
        } else {
            laserEnabled = true;
        }
        
        currentPointIndex++;
    } else {
        // Carácter completado, preparar siguiente
        currentCharPath.clear();
        currentPointIndex = 0;
        currentCharIndex++;
        
        // Si hay más caracteres, continuar procesando
        if (currentCharIndex < textToDraw.length()) {
            enterState(PROCESSING_TEXT);
        }
    }
}