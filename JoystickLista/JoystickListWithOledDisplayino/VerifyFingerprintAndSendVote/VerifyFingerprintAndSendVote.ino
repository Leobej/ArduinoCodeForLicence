#include <Adafruit_Fingerprint.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "base64.hpp"
#include <algorithm>


// const char* ssid = "Pixel_5101";
const char* ssid = "TP-Link_9A9C";
const char* password = "1234567890";
// const char* mqtt_server = "192.168.147.236";
const char* mqtt_server = "192.168.0.102";
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3D

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (1024)
char msg[MSG_BUFFER_SIZE];
int value = 0;
String payloadStr;
String deviceId;

SoftwareSerial mySerial(D5, D6);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
uint8_t id;

const int ITEM_DISPLAY_TIME = 2000;
const int JOYSTICK_X = A0;
const int JOYSTICK_Y = D3;
const int JOYSTICK_BUTTON = D7;
const int JOYSTICK_DEAD_ZONE = 50;
int current_item = 0;
int fingerprintId = -1;
int electionId = -1;
int locationId = 1;

bool isFingerprintVerified = false;
bool isRegModeOn = false;
bool isVoteModeOn = true;

DynamicJsonDocument doc(1024);
int chunkSize = 128;

struct Candidate {
  int id;
  String name;
};

Candidate candidates[10];
int numCandidates = 0;

void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  char payloadWithNull[length + 1];
  memcpy(payloadWithNull, payload, length);
  payloadWithNull[length] = '\0';
  payloadStr = String(payloadWithNull);
  Serial.println();
  Serial.println("This is the received messaged in a String: ");
  Serial.println(payloadStr);
  if (String(topic) == "voteFingerprintTopic") {
    DynamicJsonDocument doc(256);
    deserializeJson(doc, payload);
    String status = doc["status"];
    int voterId = doc["voterId"];
    electionId = doc["electionId"];
    locationId = doc["locationId"];
    if (status == "verified") {
      isFingerprintVerified = true;
      fingerprintId = voterId;
      Serial.print("Fingerprint verified for voter ID: ");
      Serial.println(voterId);
    }
    if (payloadStr == "reg") {
      isRegModeOn = true;
      isVoteModeOn = false;
      Serial.println("Register Mode is on");
    }
    if (payloadStr == "vote") {
      isRegModeOn = false;
      isVoteModeOn = true;
      Serial.println("Vote Mode is on");
    }
    if (doc.containsKey("candidates")) {
      JsonArray arr = doc["candidates"].as<JsonArray>();
      numCandidates = 0;
      for (JsonVariant v : arr) {
        candidates[numCandidates].id = v["id"];
        candidates[numCandidates].name = v["name"].as<String>();
        numCandidates++;
      }
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("voteFingerprintTopic");
      client.subscribe("voteFingerprint");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  Serial.begin(115200);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  finger.begin(57600);
  pinMode(JOYSTICK_BUTTON, INPUT_PULLUP);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();       // Clear the display buffer
  display.setTextSize(1);       // Normal 1:1 pixel scale
  display.setTextColor(WHITE);  // Draw white text
  display.setCursor(0, 0);      // Start at top-left corner
  display.println(F("System Starting..."));
  display.display();  // Show the message on the display
  delay(2000);
  setup_wifi();
}

void publishFingerprintId() {
  delay(200);
  Serial.println("In publishMessage");
  deviceId = "VOTE_1";
  doc["deviceId"] = deviceId;
  doc["fingerprint"] = fingerprintId;

  memset(msg, 0, sizeof(msg));
  serializeJson(doc, msg);
  client.publish("voteFingerprint", msg);
  client.flush();
  doc.clear();
}

void publishVoter() {
  Serial.println("In publishVoter");
  memset(msg, 0, sizeof(msg));
  deviceId = "REG_1";
  doc["fingerprint"] = fingerprintId;
  doc["deviceId"] = deviceId;

  serializeJson(doc, msg);
  client.publish("registerVoter", msg);

  client.flush();
  doc.clear();
}

bool voteSent = false;

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (payloadStr == "nextFingerprint" && isRegModeOn == true) {
    Serial.println("Ready to register a voter!");
    id = getUnusedFingerprintId();
    Serial.println(id);
    while (!getFingerprintEnroll())
      ;

    publishVoter();
    isRegModeOn = false;
    isVoteModeOn = false;
    payloadStr = "";
  }

  if (payloadStr == "nextFingerprint" && isVoteModeOn == true) {
    Serial.println("Ready to match a fingerprint!");
    while (!getFingerprintID())
      ;

    publishFingerprintId();
    isRegModeOn = false;
    isVoteModeOn = false;
    payloadStr = "";
  }

  payloadStr = "WaitingForNextFingerPrintCommand";


  int joystick_x = analogRead(JOYSTICK_X);
  int joystick_button = digitalRead(JOYSTICK_BUTTON);
  if (!voteSent) {
    // Navigation logic
    if (joystick_x < JOYSTICK_DEAD_ZONE) {
      if (current_item > 0) {
        current_item--;
      }
      delay(300);
    } else if (joystick_x > 1023 - JOYSTICK_DEAD_ZONE) {
      if (current_item < sizeof(candidates) / sizeof(candidates[0]) - 1) {
        current_item++;
      }
      delay(300);
    }
    // Display items and the arrow
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    int itemsPerPage = 5;
    int page = current_item / itemsPerPage;
    int startIndex = page * itemsPerPage;
    int endIndex = min(startIndex + itemsPerPage, numCandidates);

    for (int i = startIndex; i < endIndex; i++) {
      display.setCursor(10, (i - startIndex) * 10);
      if (i == current_item) {
        display.print("> ");  // Arrow for the selected item
      }
      display.println(candidates[i].name);
    }

    display.display();
  }

  if (isFingerprintVerified && joystick_button == LOW) {
    Serial.println("Now sending the vote");
    delay(300);
    voteSent = true;

    if (current_item >= 0 && current_item < numCandidates) {
      DynamicJsonDocument voteDoc(256);
      voteDoc["voterId"] = fingerprintId;
      voteDoc["candidateId"] = candidates[current_item].id;
      voteDoc["electionId"] = electionId;
      voteDoc["locationId"] = locationId;

      char voteMsg[256];
      serializeJson(voteDoc, voteMsg);
      delay(500);
      client.publish("send/vote", voteMsg);
      isFingerprintVerified = false;

      Serial.println("Vote sent: ");
      Serial.println(voteMsg);

      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Vote sent");
      display.println("successfully!");
      display.display();
      delay(2000);
      numCandidates = 0;
    } else {
      Serial.println("Invalid candidate selection");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Invalid selection.");
      display.display();
      delay(2000);
    }
    voteSent = false;
  }
}


uint8_t getFingerprintEnroll() {
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #");
  Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error ");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error ");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID ");
  Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print("w");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error ");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error ");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");
  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
    fingerprintId = id;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error ");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID ");
  Serial.println(id);
  p = finger.storeModel(id);
  Serial.println("finger");
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error ");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }
  Serial.println("");

  //here send message

  return true;
}

uint8_t getFingerprintID() {
  int p = -1;
  Serial.print("Waiting for valid finger to find");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error ");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }
  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #");
  // Serial.print(finger.fingerID);
  fingerprintId = finger.fingerID;
  Serial.print(fingerprintId);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);

  return fingerprintId;
}

uint8_t getUnusedFingerprintId() {
  uint8_t emptyId = 0;

  for (uint8_t i = 1; i < 15; i++) {
    uint8_t status = finger.loadModel(i);

    Serial.print(i);
    Serial.print(" ");
    Serial.print(status);
    Serial.println();
    if (status != FINGERPRINT_OK) {
      emptyId = i;
      Serial.print("Empty ID found: ");
      Serial.println(emptyId);
      return emptyId;
    }
  }

  if (emptyId == 0) {
    Serial.println("No empty ID found");
  }
  return emptyId;
}
