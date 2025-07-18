/**
 * ESP32 ADC Web Server
 * 
 * This code creates a web server that continuously displays ADC readings
 * from the ESP32's analog pin (GPIO 34). The webpage automatically updates
 * the readings using AJAX.
 */

#include <WiFi.h>
#include <WebServer.h>

// WiFi credentials - Replace with your network credentials
const char* ssid = "iMac de Unalmed";
const char* password = "fotonica109";

// Web server on port 80
WebServer server(80);

// ADC pin
const int adcPin = 34;  // GPIO 34 (Analog ADC1_CH6)
unsigned long lastSampleTime = 0;
const int sampleInterval = 300;  // Sample every 1 second

// HTML and JavaScript for the webpage with auto-refresh
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>  
<head>
  <title>ESP32 ADC Data</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      margin: 0;
      padding: 20px;
      background-color: #f5f5f5;
    }
    .container {
      max-width: 600px;
      margin: 0 auto;
      background-color: white;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
    }
    h1 {
      color: #0066cc;
    }
    .data-display {
      font-size: 24px;
      font-weight: bold;
      margin: 20px 0;
      padding: 15px;
      background-color: #f0f8ff;
      border-radius: 5px;
    }
    .chart-container {
      width: 100%;
      height: 250px;
      margin: 20px 0;
      background-color: #f0f8ff;
      border-radius: 5px;
      position: relative;
    }
    .chart {
      width: 100%;
      height: 200px;
      overflow: hidden;
      position: relative;
    }
    #values-line {
      height: 100%;
      position: absolute;
      bottom: 0;
      width: 100%;
    }
    .data-point {
      position: absolute;
      width: 4px;
      background-color: #0066cc;
      bottom: 0;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 ADC Data Monitor</h1>
    <p>Real-time ADC reading from GPIO 34</p>
    
    <div class="data-display">
      ADC Value: <span id="adcValue">--</span>
    </div>
    
    <div class="data-display">
      Voltage: <span id="voltage">--</span> V
    </div>
    
    <div class="chart-container">
      <h3>Recent Readings</h3>
      <div class="chart">
        <div id="values-line"></div>
      </div>
    </div>
  </div>
  
  <script>
    let dataPoints = [];
    const maxPoints = 100;
    
  async function updateADCValue() {
      try {
        const response = await fetch('/readADC');
        if (!response.ok) {
          throw new Error('HTTP error! Status: ${response.status}');
        }
        // Veamos que siempre lo ideal es asegurarse de que el dato llego a salvo a la pagina web, por lo que cualquier peticion de dato a pesar
        // de que el fetch se completo con exito debe de esperarse.
        const data = await response.text();
        const adcValue = parseInt(data);
        
        document.getElementById("adcValue").innerText = adcValue;
        
        // Calculate voltage (ESP32 ADC is 12-bit, so max value is 4095)
        // Reference voltage is 3.3V
        const voltage = (adcValue / 4095.0 * 3.3).toFixed(2);
        document.getElementById("voltage").innerText = voltage;
        
        // Update chart
        dataPoints.push(adcValue);
        if (dataPoints.length > maxPoints) {
          dataPoints.shift();
        }
        updateChart();
      } catch (error) {
        console.error('Fetch error:', error);
      }
    }
    
    function updateChart() {
      const chartContainer = document.getElementById("values-line");
      chartContainer.innerHTML = "";
      
      const max = 4095;  // Max ADC value
      const width = chartContainer.offsetWidth;
      const pointWidth = width / maxPoints;
      
      for (let i = 0; i < dataPoints.length; i++) {
        const point = document.createElement("div");
        point.className = "data-point";
        point.style.left = (i * pointWidth) + "px";
        point.style.height = (dataPoints[i] / max * 100) + "%";
        chartContainer.appendChild(point);
      }
    }
    
    // Update value every second
    setInterval(updateADCValue, 300);
    
    // Initial update
    updateADCValue();
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  
  // ADC setup - ESP32 has 12-bit ADC resolution (0-4095)
  analogReadResolution(12);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Route for root / web page
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", index_html);
  });
  
  // Route to get ADC data
  server.on("/readADC", HTTP_GET, []() {
    int adcValue = analogRead(adcPin);
    server.send(200, "text/plain", String(adcValue));
  });
  
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Handle client requests
  server.handleClient();
  
  // Sample and log to serial at regular intervals
  unsigned long currentMillis = millis();
  if (currentMillis - lastSampleTime >= sampleInterval) {
    lastSampleTime = currentMillis;
    int adcValue = analogRead(adcPin);
    float voltage = adcValue * 3.3 / 4095.0;
    
    Serial.print("ADC Raw Value: ");
    Serial.print(adcValue);
    Serial.print(", Voltage: ");
    Serial.println(voltage);
  }
  
  // Small delay to prevent CPU hogging
  delay(10);
}