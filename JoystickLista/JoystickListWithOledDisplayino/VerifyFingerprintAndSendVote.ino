#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Fingerprint.h>
#include <ArduinoJson.h>
const char* ssid = "310";
const char* password = "1234567890";
const char* mqtt_server = "192.168.24.236";
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

uint8_t fingerTemplateToSend[512];
char fingerTemplateHex[512];
SoftwareSerial mySerial(D6, D5);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
uint8_t id;

const char* items[] = { "Item 1", "Item 2", "Item 3", "Item 4", "Item 5" };
const int ITEM_DISPLAY_TIME = 2000;
const int JOYSTICK_X = A0;
const int JOYSTICK_Y = D3;
const int JOYSTICK_BUTTON = D7;
const int JOYSTICK_DEAD_ZONE = 50;
int current_item = 0;

DynamicJsonDocument doc(1024);
int chunkSize = 128;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
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
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  // code added by me
  char payloadWithNull[length + 1];
  memcpy(payloadWithNull, payload, length);
  payloadWithNull[length] = '\0';
  payloadStr = String(payloadWithNull);
  // till here
  Serial.println();
  Serial.println("This is the received messaged in a String: ");
  Serial.println(payloadStr);
}

void reconnect() {
  // Loop until we're reconnected

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("test/topic", "s-a reconectat");
      // ... and resubscribe
      client.subscribe("inTopic");
      // client.subscribe("verification");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);  // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  // set the data rate for the sensor serial port
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  finger.begin(57600);
  pinMode(JOYSTICK_BUTTON, INPUT_PULLUP);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  delay(1000);
  display.setTextSize(2);
  display.setTextColor(WHITE);
}

void publishMessage() {
  memset(msg, 0, sizeof(msg));

  int len = strlen(fingerTemplateHex);
  int numChunks = (len + chunkSize - 1) / chunkSize;  // Calculate number of chunks

  for (int i = 0; i < numChunks; i++) {
    char chunk[chunkSize + 1];
    strncpy(chunk, &fingerTemplateHex[i * chunkSize], chunkSize);  // Extract chunk from fingerTemplateHex
    chunk[chunkSize] = '\0';                                       // Add null terminator
    String name = "Fingerprint";
    doc["cnp"] = 1;
    doc[name] = chunk;

    serializeJson(doc, msg);
    client.publish("test/topic", msg);

    client.flush();
    doc.clear();
  }

  Serial.println("S-o trimis");
}


void loop() {


  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (payloadStr == "nextFingerprint") {
    finger.emptyDatabase();
    Serial.println("Ready to enroll a fingerprint!");
    id = 1;
    Serial.print("Enrolling ID #");
    Serial.println(id);

    while (!getFingerprintEnroll())
      ;
    publishMessage();
  }

  payloadStr = "WaitingForNextFingerPrintCommand";

  int joystick_x = analogRead(JOYSTICK_X);
  int joystick_y = digitalRead(JOYSTICK_Y);
  int joystick_button = digitalRead(JOYSTICK_BUTTON);

  if (joystick_x < JOYSTICK_DEAD_ZONE) {
    if (current_item > 0) {
      current_item--;
    }
    delay(300);  // debounce
  } else if (joystick_x > 1023 - JOYSTICK_DEAD_ZONE) {
    if (current_item < sizeof(items) / sizeof(items[0]) - 1) {
      current_item++;
    }
    delay(300);  // debounce
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(items[current_item]);
  display.display();
  int button_state = digitalRead(JOYSTICK_BUTTON);
  if (button_state == LOW) {
    delay(300);
    client.publish("send/vote", items[current_item]);
    return;
  }
}




uint8_t downloadFingerprintTemplate(uint16_t id) {
  Serial.println("------------------------------------");
  Serial.print("Attempting to load #");
  Serial.println(id);
  uint8_t p = finger.loadModel(id);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.print("Template ");
      Serial.print(id);
      Serial.println(" loaded");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error downloadFingerPrintTemplate198");
      return p;
    default:
      Serial.print("Unknown error ");
      Serial.println(p);
      return p;
  }

  // OK success!

  Serial.print("Attempting to get #");
  Serial.println(id);
  p = finger.getModel();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.print("Template ");
      Serial.print(id);
      Serial.println(" transferring:");
      break;
    default:
      Serial.print("Unknown error ");
      Serial.println(p);
      return p;
  }

  // one data packet is 267 bytes. in one data packet, 11 bytes are 'usesless' :D
  uint8_t bytesReceived[534];  // 2 data packets
  memset(bytesReceived, 0xff, 534);

  uint32_t starttime = millis();
  int i = 0;
  while (i < 534 && (millis() - starttime) < 20000) {
    if (mySerial.available()) {
      bytesReceived[i++] = mySerial.read();
    }
  }
  Serial.print(i);
  Serial.println(" bytes read.");
  Serial.println("Decoding packet...");

  uint8_t fingerTemplate[512];  // the real template
  memset(fingerTemplate, 0xff, 512);

  // filtering only the data packets
  int uindx = 9, index = 0;
  memcpy(fingerTemplate + index, bytesReceived + uindx, 256);  // first 256 bytes
  uindx += 256;                                                // skip data
  uindx += 2;                                                  // skip checksum
  uindx += 9;                                                  // skip next header
  index += 256;                                                // advance pointer
  memcpy(fingerTemplate + index, bytesReceived + uindx, 256);  // second 256 bytes

  for (int i = 0; i < 512; ++i) {
    //Serial.print("0x");
    printHex(fingerTemplate[i], 2);
    //Serial.print(", ");
  }
  Serial.println("\ndone.");

  toHexString(fingerTemplate, sizeof(fingerTemplate), fingerTemplateHex, sizeof(fingerTemplateHex));
  Serial.println("ceva");
  Serial.print(fingerTemplateHex);
  Serial.println("\naltceva");
  return p;
}


void toHexString(uint8_t* data, uint16_t size, char* hexString, uint16_t maxStringSize) {
  char hexLookup[] = "0123456789ABCDEF";
  uint16_t i = 0, j = 0;
  for (i = 0; i < size && j + 2 < maxStringSize; i++) {
    hexString[j++] = hexLookup[(data[i] >> 4) & 0x0F];
    hexString[j++] = hexLookup[data[i] & 0x0F];
  }
  hexString[j] = '\0';  // terminate the string
}


void printHex(int num, int precision) {
  char tmp[16];
  char format[128];

  sprintf(format, "%%.%dX", precision);

  sprintf(tmp, format, num);

  Serial.print(tmp);
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
  downloadFingerprintTemplate(id);

  Serial.println("Exit from getFingerprintEnroll()");

  return true;
}

uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
    while (!Serial.available())
      ;
    num = Serial.parseInt();
  }
  return num;
}
