/* @file HelloKeypad.pde
|| @version 1.0
|| @author Alexander Brevig
|| @contact alexanderbrevig@gmail.com
||
|| @description
|| | Demonstrates the simplest use of the matrix Keypad library.
|| #
*/
#include "Keypad.h"
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

// For Arduino Microcontroller
// byte rowPins[ROWS] = {9, 8, 7, 6}; 
// byte colPins[COLS] = {5, 4, 3, 2}; 

// For ESP8266 Microcontroller
byte rowPins[ROWS] = {D0, D1, D2, D3}; 
byte colPins[COLS] = {D4, D5, D6 }; 

// For ESP32 Microcontroller
//byte rowPins[ROWS] = {23, 22, 3, 21}; 
//byte colPins[COLS] = {19, 18, 5, 17};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(9600);
}

void loop() {
  char key = keypad.getKey();

  if (key){
    Serial.println(key);
  }
}




// const byte n_rows = 4;
// const byte n_cols = 4;
// char keys[n_rows][n_cols] = {

//   { '1', '2', '3', 'A' },

//   { '4', '5', '6', 'B' },

//   { '7', '8', '9', 'C' },

//   { '*', '0', '#', 'D' }

// };
// byte colPins[n_rows] = { D3, D2, D1, D0 };
// byte rowPins[n_cols] = { D7, D6, D5, D4 };  //connect to the column pinouts of the keypad
// LiquidCrystal_I2C lcd(0x27, 16, 2);         // I2C address, SDA, SCL, backlight (optional)

// Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, n_rows, n_cols);
// String enteredData = "";
// void setup() {
//   // Wire.begin(SDA, SCL);
//   Serial.begin(115200);
//   Wire.begin(D1, D2);  // initialize I2C bus with SDA=D2 and SCL=D1
//   lcd.init();          // initialize the LCD display
//   lcd.backlight();
// }
// void loop() {
//   // char key = keypad.getKey();
//   // lcd.setCursor(0, 0);
//   // lcd.print("cute");
//   // if (key != NO_KEY) {
//   //   if (key == '#') {
//   //     Serial.println(enteredData);
//   //     lcd.clear();
//   //     lcd.print(enteredData);
//   //     enteredData = "";
//   //   } else if (key == 'D') {
//   //     if (enteredData.length() > 0) {
//   //       enteredData.remove(enteredData.length() - 1);
//   //       lcd.setCursor(0, 0);
//   //       lcd.print(enteredData);
//   //     }
//   //   } else {
//   //     enteredData += key;
//   //     lcd.setCursor(0, 0);
//   //     lcd.print(enteredData);
//   //   }
//   // }
//   Serial.println("Sal");
// }
