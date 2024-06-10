#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* ssid = "tang2";
const char* password = "12345678";
const char* mqtt_server = "192.168.31.254";
const char* mqtt_user = "mqtt_user";
const char* mqtt_pass = "12345678";

WiFiClient espClient;
PubSubClient client(espClient);

void handleSensorDataFromBB(String message) {
  DynamicJsonDocument doc(256);
  int tempIndex = message.indexOf("Temp: ");
  int humiIndex = message.indexOf("Humidity: ");
  int lightIndex = message.indexOf("Light: ");
  int pirIndex = message.indexOf("PIR: ");
  
  if (tempIndex != -1 && humiIndex != -1 && lightIndex != -1 && pirIndex != -1) {
    String temp = message.substring(tempIndex + 6, message.indexOf("C", tempIndex));
    String humi = message.substring(humiIndex + 10, message.indexOf("%", humiIndex));
    String light = message.substring(lightIndex + 7, message.indexOf(",", lightIndex));
    String pir = message.substring(pirIndex + 5, message.indexOf(" ", pirIndex + 5));

    doc["temperature"] = temp;
    doc["humidity"] = humi;
    doc["light"] = light;
    doc["pir"] = pir;
  }

  char buffer[256];
  size_t n = serializeJson(doc, buffer);
  client.publish("home-assistant/sensor1", buffer, n);
}

void handleSensorDataFromBA(String message) {
  DynamicJsonDocument doc(256);
  int tempIndex = message.indexOf("Temp: ");
  int humiIndex = message.indexOf("Humidity: ");
  int lightIndex = message.indexOf("Light: ");
  int fanIndex = message.indexOf("Fan: ");
  
  if (tempIndex != -1 && humiIndex != -1 && lightIndex != -1 && fanIndex != -1) {
    String temp = message.substring(tempIndex + 6, message.indexOf("C", tempIndex));
    String humi = message.substring(humiIndex + 10, message.indexOf("%", humiIndex));
    String light = message.substring(lightIndex + 7, message.indexOf(",", lightIndex));
    String fan = message.substring(fanIndex + 5, message.indexOf(",", fanIndex));

    doc["temperature"] = temp;
    doc["humidity"] = humi;
    doc["light"] = light;
    doc["fan"] = fan;
  }

  char buffer[256];
  size_t n = serializeJson(doc, buffer);
  client.publish("home-assistant/sensor2", buffer, n);
}

void sendCommandToZigbee(String command) {
  Serial2.println(command);
  Serial.println("Command sent to Zigbee: " + command);
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  Serial.println(messageTemp);

  if (String(topic) == "zigbee/command") {
    sendCommandToZigbee(messageTemp);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.subscribe("zigbee/command");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, 16, 17); 

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.println("Receiver is ready.");
}
void sendSensorData(String message) {
  if (message.indexOf("From Add: 0xbb") != -1) {
    handleSensorDataFromBB(message);
  } else if (message.indexOf("From Add: 0xba") != -1) {
    handleSensorDataFromBA(message);
  } else {
    Serial.println("Unknown address in message: " + message);
  }
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (Serial2.available()) {
    String message = Serial2.readStringUntil('\n');
    Serial.println(message);
    sendSensorData(message);
  }
}

