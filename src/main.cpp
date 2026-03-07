#include <Arduino.h>

// Constants 
#define STATE_OFF 0
#define STATE_FIDGET 1
#define STATE_SCROLL 2


// put function declarations here:
int myFunction(int, int);
void setStatusColor(int r, int g, int b);

// Pins
int xPin = A0;
int yPin = A1;
int stickbuttonPin = 2;
int buttonPin = 3;

// Individual Light
int red = 11;
int yellow = 12;
int green = 13;

// RGB Light
int rgb_RED = 6;
int rgb_GREEN = 10;
int rgb_BLUE = 9;


// Tracking Variables
int currentState = STATE_OFF;
unsigned long pressStartTime = 0;
bool isHolding = false;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // JoyStick Button Press
  pinMode(stickbuttonPin,INPUT_PULLUP);
  // Mode/Power Button
  pinMode(buttonPin,INPUT_PULLUP); 

  // Lights
  pinMode(red,OUTPUT); 
  pinMode(yellow,OUTPUT);
  pinMode(green,OUTPUT);
  pinMode(rgb_RED,OUTPUT);
  pinMode(rgb_GREEN,OUTPUT);
  pinMode(rgb_BLUE,OUTPUT);


  // int result = myFunction(2, 3);
}

void loop() {
  // put your main code here, to run repeatedly:
  int xValue = analogRead(xPin);
  int yValue = analogRead(yPin);
  int stickbValue = digitalRead(stickbuttonPin);

  switch (currentState) {
    case STATE_OFF:
      
    break;
    case STATE_FIDGET:
      
    break;
    case STATE_SCROLL:
      
    break;
  } 

  // // RIGHT
  // if (xValue > 800) {
  //   digitalWrite(11, HIGH);
  // } else {
  //   digitalWrite(11, LOW);
  // }
  // if (stickbValue == LOW) {
  //   digitalWrite(12, HIGH);
  // } else {
  //   digitalWrite(12, LOW);
  // }
  // // LEFT
  // if (xValue < 200) {
  //   digitalWrite(13, HIGH);
  // } else {
  //   digitalWrite(13, LOW);
  // }

}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}