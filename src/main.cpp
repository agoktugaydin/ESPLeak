#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <config.h>

IPAddress webSocketServer(WEBSOCKET_SERVER_IP1, WEBSOCKET_SERVER_IP2, WEBSOCKET_SERVER_IP3, WEBSOCKET_SERVER_IP4);
int webSocketPort = WEBSOCKET_SERVER_PORT;
const char * ssid = WIFI_SSID;
const char * password = WIFI_PASSWORD;

WebSocketsClient wsClient;

const int analogIn = A0;
int rawValue = 0;
double voltage = 0;
double rawSum = 0;
int limit = 2000;
int percentage = 0;

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

typedef struct struct_message {
  int id;
  int x;
  int y;
}
struct_message;

struct_message myData;
struct_message board1;
struct_message boardsStruct[1] = {
  board1
};

void OnDataRecv(const uint8_t * mac_addr,
  const uint8_t * incomingData, int len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
    mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy( & myData, incomingData, sizeof(myData));
  Serial.printf("Board ID %u: %u bytes\n", myData.id, len);
  boardsStruct[myData.id - 1].x = myData.x;
  boardsStruct[myData.id - 1].y = myData.y;
  Serial.printf("x value: %d \n", boardsStruct[myData.id - 1].x);
  Serial.printf("y value: %d \n", boardsStruct[myData.id - 1].y);
  Serial.println("Data received from ESP-NOW");

}

void displayValues() {
  rawSum = analogRead(analogIn);

  for (int i = 0; i < 500; i++) {
    rawSum += analogRead(analogIn);
  }

  rawValue = rawSum / 500;
  voltage = (rawValue / 4096.0) * 3300;
  percentage = map(rawValue, 1400, 4096, 0, 100);

  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);

  if (rawValue < limit) {
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(18, LOW);
    display.print("NORMAL");
    display.setCursor(0, 10);
    display.print("raw value:");
    display.print(rawValue);
    display.print("\n");
    display.print("received value:");
    display.print(boardsStruct[0].x);
    display.print("\n");
    display.print("Percentage: ");
    display.print(percentage);
    display.print("%");
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(18, HIGH);
    display.print("LEAK");
    display.setCursor(0, 10);
    display.print("raw value:");
    display.print(rawValue);
    display.print("\n");
    display.print("received value:");
    display.print(boardsStruct[0].x);
    display.print("\n");
    display.print("Percentage: ");
    display.print(percentage);
    display.print("%");

  }

  display.display();
  delay(300);
  display.clearDisplay();
}

void sendData() {

  StaticJsonDocument < 200 > jsonData;

  jsonData["deviceId"] = "3bf76280-6ca9-4d83-9ffb-db112de00c24";
  jsonData["gasIntensity"] = percentage;
  jsonData["zone"] = "43C72";

  String serializedData;
  serializeJson(jsonData, serializedData);

  wsClient.sendTXT(serializedData);
  Serial.println(serializedData);
}

void wsEvent(WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_DISCONNECTED) {
    Serial.println("Disconnected from WebSocket server.");
  } else if (type == WStype_CONNECTED) {
    Serial.println("Connected to WebSocket server.");
  } else if (type == WStype_TEXT) {
    Serial.println("Received data from WebSocket server:");
    Serial.println((char * ) payload);
  }
}

void setup() {

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(18, OUTPUT);

  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  delay(300);
  // WiFi.begin(ssid, password);

  wsClient.begin(webSocketServer, webSocketPort);
  wsClient.onEvent(wsEvent);
  Serial.println("WebSocket client started...");
  
}

void loop() {
  displayValues();
  delay(300);
  wsClient.loop();
  static unsigned long lastSent = 0;
  unsigned long now = millis();

  if (wsClient.isConnected() && now - lastSent >= 2000) {
    sendData();
  }

}