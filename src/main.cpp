#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <config.h>

const char* ssid =  WIFI_SSID;
const char* password = WIFI_PASSWORD;

// IP address and port number of the WebSocket server
IPAddress webSocketServer(192, 168, 1, 36);
int webSocketPort = 5600;

WebSocketsClient wsClient;

void sendData() {
  // JSON object to hold the data
  StaticJsonDocument<200> jsonDoc;
  
  jsonDoc["message"] = "Hello from ESP32";
  jsonDoc["millis"] = millis();

  // Serialize the JSON to a buffer
  char buffer[200];
  serializeJson(jsonDoc, buffer);

  // Send the JSON data over WebSocket
  wsClient.sendTXT(buffer);
}


void wsEvent(WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_DISCONNECTED) {
    Serial.println("Disconnected from WebSocket server.");
  } else if (type == WStype_CONNECTED) {
    Serial.println("Connected to WebSocket server.");
  } else if (type == WStype_TEXT) {
    Serial.println("Received data from WebSocket server:");
    Serial.println((char*)payload);
  }
}

void setup() {
  // Serial port is initialized for debugging
  Serial.begin(115200);

  // Connect to WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Start WebSocket connection and specify the event function
  wsClient.begin(webSocketServer, webSocketPort);
  wsClient.onEvent(wsEvent);

  Serial.println("WebSocket client started...");
}

void loop() {
  // Perform WebSocket operations
  wsClient.loop();
  static unsigned long lastSent = 0;
  unsigned long now = millis();

  // Send data at regular intervals
  if (wsClient.isConnected() && now - lastSent >= 2000) {
    sendData();
    lastSent = now;
  }
}
