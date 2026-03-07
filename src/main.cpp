#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);

int xPin = A0;
int yPin = A1;
int buttonPin = 2;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // Button Press
  pinMode(buttonPin,INPUT_PULLUP);
  // Lights
  pinMode(11,OUTPUT); // Red
  pinMode(12,OUTPUT); // Yellow
  pinMode(13,OUTPUT); // Green
  // int result = myFunction(2, 3);
}

void loop() {
  // put your main code here, to run repeatedly:
  // put your main code here, to run repeatedly:
  int xValue = analogRead(xPin);
  int yValue = analogRead(yPin);
  int bValue = digitalRead(buttonPin);
  // RIGHT
  if (xValue > 800) {
    digitalWrite(11, HIGH);
  } else {
    digitalWrite(11, LOW);
  }
  if (bValue == LOW) {
    digitalWrite(12, HIGH);
  } else {
    digitalWrite(12, LOW);
  }
  // LEFT
  if (xValue < 200) {
    digitalWrite(13, HIGH);
  } else {
    digitalWrite(13, LOW);
  }
  

}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}