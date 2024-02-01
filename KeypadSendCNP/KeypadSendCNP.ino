
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Keypad.h>
#include <Adafruit_SSD1306.h>


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WiFi and MQTT credentials
// const char* ssid = "Pixel_5101";
const char* ssid = "TP-Link_9A9C";
const char* password = "1234567890";
// const char* mqtt_server = "192.168.147.236";
const char* mqtt_server = "192.168.0.102";

// Keypad configuration
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};
StaticJsonDocument<200> doc;
char deviceId[] = "deviceId1";
byte rowPins[ROWS] = { D0, D1, D2, D3 };
byte colPins[COLS] = { D4, D5, D6 };
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

// Message buffer
const int messageLength = 13;
char message[messageLength + 1];
int messageIndex = 0;

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected to WiFi");

  client.setServer(mqtt_server, 1883);
  // display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  // display.clearDisplay();
  // display.setTextSize(1);
  // display.setTextColor(WHITE);
  // display.setCursor(0, 0);
  // display.println(F("System Starting..."));
  // display.display();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  char key = keypad.getKey();
  if (key != NO_KEY) {
    if (key == '*') {
      if (messageIndex > 0) {
        messageIndex--;
        message[messageIndex] = '\0';
        Serial.print("Deleted: ");
        Serial.println(message);
        // display.print(message);
      }
    } else if (key == '#') {
      if (messageIndex == messageLength) {
        doc["cnp"] = message;
        doc["deviceId"] = deviceId;
        char jsonOutput[256];  // Adjust the size as needed
        serializeJson(doc, jsonOutput);

        client.publish("cnp", jsonOutput);
        Serial.print("Message sent: ");
        Serial.println(message);
        Serial.println(jsonOutput);


      } else {
        Serial.println("Message must be 13 characters long.");
      }
    } else {
      if (messageIndex < messageLength) {
        message[messageIndex++] = key;
        message[messageIndex] = '\0';
        Serial.print("Input: ");
        Serial.println(message);

      } else {
        Serial.println("Maximum input length reached. Press '*' to delete or '#' to send.");
      }
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
