/*
 Basic ESP8266 MQTT example
 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off
 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json1
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Fingerprint.h>
#include <ArduinoJson.h>
// Update these with values suitable for your network.

const char* ssid = "DIGI_bb4ed4";
const char* password = "1234567890";
const char* mqtt_server = "192.168.1.9";


#define MSG_BUFFER_SIZE (1024)


WiFiClient espClient;
PubSubClient client(espClient);
char msg[MSG_BUFFER_SIZE];
int value = 0;
String messageFromServer;
bool enrolled = false;

uint8_t fingerTemplateToSend[512];
char fingerTemplateHex[512];

String deviceId="REG_1";

//for AS608 fingerprint
SoftwareSerial mySerial(D6, D5);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
uint8_t id;


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
  messageFromServer = String(payloadWithNull);
  // till here
  Serial.println();
  Serial.println("This is the received messaged in a String: ");
  Serial.println(messageFromServer);
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
}

//Arduinojson library
DynamicJsonDocument doc(1024);
int key = 0;
int chunkSize = 128;

void publishMessage() {
  memset(msg, 0, sizeof(msg));

  int len = strlen(fingerTemplateHex);
  int numChunks = (len + chunkSize - 1) / chunkSize;  // Calculate number of chunks

  for (int i = 0; i < numChunks; i++) {
    char chunk[chunkSize + 1];
    strncpy(chunk, &fingerTemplateHex[i * chunkSize], chunkSize);  // Extract chunk from fingerTemplateHex
    chunk[chunkSize] = '\0';                                       // Add null terminator
    doc["key"] = key;
    doc["fingerprint"] = chunk;
    doc["deviceId"]=deviceId;

    serializeJson(doc, msg);
    client.publish("test/topic", msg);

    client.flush();
    doc.clear();
  }
  key++;
  Serial.println("S-o trimis");
  enrolled = true;
}




void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (messageFromServer
      == "nextFingerprint") {
    finger.emptyDatabase();
    Serial.println("Ready to enroll a fingerprint!");
    id = 1;
    Serial.print("Enrolling ID #");
    Serial.println(id);

    while (!getFingerprintEnroll())
      ;
    publishMessage();
  }

  messageFromServer = "WaitingForNextFingerPrintCommand";
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
