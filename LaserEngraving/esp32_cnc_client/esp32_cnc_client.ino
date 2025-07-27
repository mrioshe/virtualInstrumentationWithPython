#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <AccelStepper.h>
#include <WebSocketsClient.h>

// --- WiFi Configuration ---
const char* ssid = "Comunidad_UNMED";
const char* password = "wifi_med_213";

// --- Replit Server Configuration ---
// Replace with your actual Replit URL (without https://)
const char* serverHost = "your-repl-name.your-username.repl.co";
const int serverPort = 443;  // Use 443 for HTTPS
const char* serverPath = "/ws";  // WebSocket path

// --- CNC Hardware Configuration ---
#define STEP1 2
#define DIR1 4
#define STEP2 12
#define DIR2 26
const int laserPin = 27;

// Sensor pins
const int sensorPin1 = 35;  // MQ135 Sensor 1
const int sensorPin2 = 34;  // MQ135 Sensor 2

// --- CNC Parameters ---
const int STEPS_PER_MM = 80;
const float MAX_SPEED = 1000.0;
const float ACCELERATION = 400.0;
const long CHAR_WIDTH_MM = 1;
const long CHAR_HEIGHT_MM = 1.5;
const long SPACE_BETWEEN_CHARS_MM = 1;

// --- Motor Objects ---
AccelStepper motorY(AccelStepper::DRIVER, STEP1, DIR1);  // Y-axis
AccelStepper motorX(AccelStepper::DRIVER, STEP2, DIR2);  // X-axis

// --- WebSocket Client ---
WebSocketsClient webSocket;

// --- Global Variables ---
String currentText = "";
bool laserEnabled = false;
bool emergencyStop = false;
float currentProgress = 0.0;
unsigned long lastSensorRead = 0;
unsigned long lastStatusUpdate = 0;
const unsigned long SENSOR_INTERVAL = 2000;  // 2 seconds
const unsigned long STATUS_INTERVAL = 1000;  // 1 second

// --- CNC State Machine ---
enum CNC_State {
  IDLE,
  PROCESSING,
  PAUSED,
  EMERGENCY
};

CNC_State currentState = IDLE;
int currentCharIndex = 0;
long xOffset = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize hardware
  setupMotors();
  setupSensors();
  setupLaser();
  
  // Connect to WiFi
  connectToWiFi();
  
  // Initialize WebSocket connection
  setupWebSocket();
  
  Serial.println("ESP32 CNC Client Ready!");
}

void loop() {
  // Handle WebSocket communication
  webSocket.loop();
  
  // Handle emergency stop
  if (emergencyStop) {
    handleEmergencyStop();
    return;
  }
  
  // Run CNC operations
  if (currentState == PROCESSING) {
    processCNCJob();
  }
  
  // Send sensor data periodically
  if (millis() - lastSensorRead > SENSOR_INTERVAL) {
    sendSensorData();
    lastSensorRead = millis();
  }
  
  // Send status updates periodically
  if (millis() - lastStatusUpdate > STATUS_INTERVAL) {
    sendStatusUpdate();
    lastStatusUpdate = millis();
  }
  
  // Run motor steps
  motorX.run();
  motorY.run();
  
  delay(1);
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
}

void setupWebSocket() {
  // Configure WebSocket client for secure connection
  webSocket.beginSSL(serverHost, serverPort, serverPath);
  
  // Set event handler
  webSocket.onEvent(webSocketEvent);
  
  // Set reconnect interval
  webSocket.setReconnectInterval(5000);
  
  Serial.println("WebSocket client configured");
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket Disconnected");
      break;
      
    case WStype_CONNECTED:
      Serial.printf("WebSocket Connected to: %s\n", payload);
      break;
      
    case WStype_TEXT:
      Serial.printf("Received: %s\n", payload);
      handleWebSocketMessage((char*)payload);
      break;
      
    case WStype_ERROR:
      Serial.printf("WebSocket Error: %s\n", payload);
      break;
      
    default:
      break;
  }
}

void handleWebSocketMessage(String message) {
  // Parse JSON message from server
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, message);
  
  String type = doc["type"];
  
  if (type == "cnc_command") {
    String command = doc["command"];
    
    if (command == "start") {
      currentText = doc["text"].as<String>();
      startEngraving();
    }
    else if (command == "pause") {
      pauseEngraving();
    }
    else if (command == "resume") {
      resumeEngraving();
    }
    else if (command == "stop") {
      stopEngraving();
    }
    else if (command == "emergency_stop") {
      emergencyStop = true;
    }
  }
}

void setupMotors() {
  motorX.setMaxSpeed(MAX_SPEED);
  motorX.setAcceleration(ACCELERATION);
  motorY.setMaxSpeed(MAX_SPEED);
  motorY.setAcceleration(ACCELERATION);
  
  // Set initial position
  motorX.setCurrentPosition(0);
  motorY.setCurrentPosition(0);
}

void setupSensors() {
  pinMode(sensorPin1, INPUT);
  pinMode(sensorPin2, INPUT);
}

void setupLaser() {
  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, LOW);
}

void startEngraving() {
  if (currentState == IDLE) {
    currentState = PROCESSING;
    currentCharIndex = 0;
    xOffset = 0;
    currentProgress = 0.0;
    
    Serial.println("Starting engraving: " + currentText);
    sendStatusUpdate();
  }
}

void pauseEngraving() {
  if (currentState == PROCESSING) {
    currentState = PAUSED;
    laserEnabled = false;
    digitalWrite(laserPin, LOW);
    
    Serial.println("Engraving paused");
    sendStatusUpdate();
  }
}

void resumeEngraving() {
  if (currentState == PAUSED) {
    currentState = PROCESSING;
    
    Serial.println("Engraving resumed");
    sendStatusUpdate();
  }
}

void stopEngraving() {
  currentState = IDLE;
  laserEnabled = false;
  digitalWrite(laserPin, LOW);
  currentProgress = 0.0;
  
  // Return to home position
  motorX.moveTo(0);
  motorY.moveTo(0);
  
  Serial.println("Engraving stopped");
  sendStatusUpdate();
}

void handleEmergencyStop() {
  currentState = EMERGENCY;
  laserEnabled = false;
  digitalWrite(laserPin, LOW);
  
  // Stop all motors immediately
  motorX.stop();
  motorY.stop();
  
  Serial.println("EMERGENCY STOP ACTIVATED!");
  sendStatusUpdate();
  
  // Reset emergency flag after sending status
  emergencyStop = false;
  currentState = IDLE;
}

void processCNCJob() {
  if (currentText.length() == 0) {
    stopEngraving();
    return;
  }
  
  // Simple character processing (you can expand this based on your character patterns)
  if (currentCharIndex < currentText.length()) {
    // Process current character
    char currentChar = currentText.charAt(currentCharIndex);
    
    // Enable laser for engraving
    laserEnabled = true;
    digitalWrite(laserPin, HIGH);
    
    // Move to character position (simplified)
    long targetX = xOffset + (CHAR_WIDTH_MM * STEPS_PER_MM);
    motorX.moveTo(targetX);
    
    // Update progress
    currentProgress = ((float)(currentCharIndex + 1) / (float)currentText.length()) * 100.0;
    
    // Move to next character when current movement is complete
    if (motorX.distanceToGo() == 0) {
      currentCharIndex++;
      xOffset += (CHAR_WIDTH_MM + SPACE_BETWEEN_CHARS_MM) * STEPS_PER_MM;
      
      // Disable laser between characters
      laserEnabled = false;
      digitalWrite(laserPin, LOW);
    }
  } else {
    // Engraving complete
    currentProgress = 100.0;
    stopEngraving();
  }
}

void sendSensorData() {
  // Read sensor values
  int sensor1Raw = analogRead(sensorPin1);
  int sensor2Raw = analogRead(sensorPin2);
  
  // Convert to PPM (simplified conversion - adjust based on your calibration)
  int sensor1Ppm = map(sensor1Raw, 0, 4095, 0, 1000);
  int sensor2Ppm = map(sensor2Raw, 0, 4095, 0, 1000);
  
  // Send HTTP POST to server
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://" + String(serverHost) + "/api/esp32/sensor-data");
    http.addHeader("Content-Type", "application/json");
    
    // Create JSON payload
    String jsonPayload = "{";
    jsonPayload += "\"sensor1Value\":" + String(sensor1Raw) + ",";
    jsonPayload += "\"sensor2Value\":" + String(sensor2Raw) + ",";
    jsonPayload += "\"sensor1Ppm\":" + String(sensor1Ppm) + ",";
    jsonPayload += "\"sensor2Ppm\":" + String(sensor2Ppm);
    jsonPayload += "}";
    
    int httpResponseCode = http.POST(jsonPayload);
    
    if (httpResponseCode > 0) {
      Serial.println("Sensor data sent successfully");
    } else {
      Serial.println("Error sending sensor data: " + String(httpResponseCode));
    }
    
    http.end();
  }
}

void sendStatusUpdate() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://" + String(serverHost) + "/api/esp32/status-update");
    http.addHeader("Content-Type", "application/json");
    
    // Get current positions
    long posX = motorX.currentPosition();
    long posY = motorY.currentPosition();
    
    // Convert state to string
    String stateStr = "idle";
    switch (currentState) {
      case PROCESSING: stateStr = "processing"; break;
      case PAUSED: stateStr = "paused"; break;
      case EMERGENCY: stateStr = "emergency"; break;
      default: stateStr = "idle"; break;
    }
    
    // Create JSON payload
    String jsonPayload = "{";
    jsonPayload += "\"state\":\"" + stateStr + "\",";
    jsonPayload += "\"positionX\":" + String(posX) + ",";
    jsonPayload += "\"positionY\":" + String(posY) + ",";
    jsonPayload += "\"progress\":" + String(currentProgress);
    jsonPayload += "}";
    
    int httpResponseCode = http.POST(jsonPayload);
    
    if (httpResponseCode > 0) {
      Serial.println("Status update sent successfully");
    } else {
      Serial.println("Error sending status update: " + String(httpResponseCode));
    }
    
    http.end();
  }
}