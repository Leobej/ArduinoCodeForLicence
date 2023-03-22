/*
 * Created by ArduinoGetStarted.com
 *
 * This example code is in the public domain
 *
 * Tutorial page: https://arduinogetstarted.com/tutorials/arduino-joystick
 */

#define VRX_PIN A0  // Arduino pin connected to VRX pin
#define VRY_PIN A1  // Arduino pin connected to VRY pin
#define SW_PIN 2    // Arduino pin connected to SW  pin
#include <ezButton.h>
int xValue = 0;  // To store value of the X axis
int yValue = 0;  // To store value of the Y axis

int selectedOption = 0;                                                     // To store the index of the currently selected option
const int numOptions = 3;                                                   // The number of options in the list
const String options[numOptions] = { "PrimaVarianta", "ADouaVarianta", "ATreiaVariatna" };  // The list of options
ezButton button(SW_PIN);
int bValue = 0;

void setup() {
  Serial.begin(9600);
  button.setDebounceTime(50);
}

void loop() {
  // read analog X and Y analog values
  xValue = analogRead(VRX_PIN);
  yValue = analogRead(VRY_PIN);
  Serial.print("x=: ");
  Serial.println(xValue);
  Serial.print("y=: ");
  Serial.print(yValue);
  Serial.println("buttonValue=: ");
  Serial.print(bValue);

  // calculate the current joystick direction
  int joystickDirection = getJoystickDirection(xValue, yValue);
  bValue = button.getState();
  // if the joystick is pressed, select the current option
  if (bValue>1) {
    Serial.print("pressed option: ");
    Serial.println(options[selectedOption]);
  }
  // if the joystick is moved left, select the previous option
  else if (joystickDirection == 1) {
    selectedOption--;
    if (selectedOption < 0) {
      selectedOption = numOptions - 1;
    }
    Serial.print("Selected option: ");
    Serial.println(options[selectedOption]);
  }
  // if the joystick is moved right, select the next option
  else if (joystickDirection == 3) {
    selectedOption++;
    if (selectedOption >= numOptions) {
      selectedOption = 0;
    }
    Serial.print("Selected option: ");
    Serial.println(options[selectedOption]);
  }

  delay(200);
}

// function to get the current joystick direction based on the X and Y values
int getJoystickDirection(int xValue, int yValue) {
  int joystickDirection = 0;
  // if (yValue < 300) {
  //   joystickDirection = 1;  // joystick moved up
  // } else if (yValue > 700) {
  //   joystickDirection = 2;  // joystick moved down
  // }
  if (xValue < 493) {
    joystickDirection = 1;  // joystick moved left
  } else if (xValue > 495) {
    joystickDirection = 3;  // joystick moved right
  }
 
  return joystickDirection;
}
