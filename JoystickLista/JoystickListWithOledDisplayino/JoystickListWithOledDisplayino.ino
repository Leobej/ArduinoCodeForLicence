#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3D

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char *items[] = { "Item 1", "Item 2", "Item 3", "Item 4", "Item 5" };
const int ITEM_DISPLAY_TIME = 2000;
const int JOYSTICK_X = A0;
const int JOYSTICK_Y = D3;
const int JOYSTICK_BUTTON = D7;
const int JOYSTICK_DEAD_ZONE = 50;
int current_item = 0;

void setup() {
  Serial.begin(9600);
  pinMode(JOYSTICK_BUTTON, INPUT_PULLUP);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  delay(1000);
  display.setTextSize(2);
  display.setTextColor(WHITE);
}

void loop() {
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
    delay(300);  // debounce
    Serial.println(items[current_item]);
    display.print(items[current_item]);
    return;
  }
}
