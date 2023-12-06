#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <config.h>


// IP address and port number of the WebSocket server
IPAddress webSocketServer(WS_SERVER);
int webSocketPort = WS_PORT;
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

WebSocketsClient wsClient;

// JSON object to hold the data
StaticJsonDocument<300> jsonDoc;

void updateSlaveData(const String& masterId, const String& slaveId, float gasDensity, float temperature) {
  // Iterate over each master object in the JSON document
  for (JsonObject master : jsonDoc.as<JsonArray>()) {
    // Check if the master ID matches the specified masterId
    if (master["master_id"].as<String>() == masterId) {
      // Iterate over each slave object within the master
      for (JsonObject slave : master["slaves"].as<JsonArray>()) {
        // Check if the slave ID matches the specified slaveId
        if (slave["slave_id"].as<String>() == slaveId) {
          // Access the data object within the slave and update the gasDensity and temperature values
          JsonObject slaveData = slave["data"].as<JsonObject>();
          slaveData["gasDensity"] = gasDensity;
          slaveData["temperature"] = temperature;
          return; // Exit the function after updating the data
        }
      }
    }
  }
}

void generateFakeData() {
  // Generate random gasDensity and temperature values
  float gasDensity = random(20, 30) + random(0, 10) / 10.0;
  float temperature = random(40, 60) + random(0, 10) / 10.0;

  // Update slave data for master ID: "1", slave ID: "1"
  updateSlaveData("1", "1", gasDensity, temperature);

  // Update slave data for master ID: "1", slave ID: "2"
  gasDensity = random(20, 30) + random(0, 10) / 10.0;
  temperature = random(40, 60) + random(0, 10) / 10.0;
  updateSlaveData("1", "2", gasDensity, temperature);
}


void sendData() {
  // Generate fake data
  generateFakeData();

  // Serialize the JSON data
  String jsonData;
  serializeJson(jsonDoc, jsonData);

  // Send the JSON data over WebSocket
  wsClient.sendTXT(jsonData);
  Serial.println(jsonData);
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

  // Clear the JSON document
  jsonDoc.clear();


  // Add an empty array to the JSON document
  JsonArray masters = jsonDoc.to<JsonArray>();

  // Add master objects to the array
  JsonObject master1 = masters.createNestedObject();
  master1["master_id"] = "1";
  JsonArray slaves1 = master1.createNestedArray("slaves");
  JsonObject slave1 = slaves1.createNestedObject();
  slave1["slave_id"] = "1";
  JsonObject slaveData1 = slave1.createNestedObject("data");
  slaveData1["gasDensity"] = 0.0;
  slaveData1["temperature"] = 0.0;
  JsonObject slave2 = slaves1.createNestedObject();
  slave2["slave_id"] = "2";
  JsonObject slaveData2 = slave2.createNestedObject("data");
  slaveData2["gasDensity"] = 0.0;
  slaveData2["temperature"] = 0.0;

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
