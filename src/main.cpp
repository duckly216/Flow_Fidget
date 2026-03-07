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
  int buttonValue = digitalRead(buttonPin);



  switch (currentState) {
    case STATE_OFF:
      // Button is being pressed and it was NOT being pressed originally
      // Serial.println("Gadget currently off");

      if(buttonValue == LOW && !isHolding){
        pressStartTime = millis();
        isHolding = true;
        // Serial.println("Button Pressing Initialized");
      }
      if(buttonValue == LOW && isHolding){
        unsigned long duration = millis() - pressStartTime;
        // At 1 seconds, turn on the first indicator
        if (duration > 1000) {digitalWrite(yellow, HIGH);}
        
        // At 3 seconds, turn on the second indicator (Yellow/RGB)
        if (duration > 3000) {digitalWrite(green, HIGH);}
        // At 5 seconds, BOOM - Power On
        if (duration >= 5000) {
            currentState = STATE_FIDGET;
            isHolding = false;
            digitalWrite(yellow, LOW);
            
            digitalWrite(green, LOW); delay(200);
            digitalWrite(green, HIGH); delay(200);
            digitalWrite(green, LOW); delay(200);
            digitalWrite(green, HIGH); delay(200); 
            digitalWrite(green, LOW);
            // Turn everything off or flash green to signal "Ready"
        }
      }
      if(buttonValue == HIGH){
        isHolding = false;
        digitalWrite(yellow, LOW);
        digitalWrite(green, LOW);
      }
      break;
    case STATE_FIDGET:
      setStatusColor(0,0,255);
    break;
    case STATE_SCROLL:
      
    break;
  } 

}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}
void setStatusColor(int r, int g, int b){
  analogWrite(rgb_RED, r);
  analogWrite(rgb_GREEN, g);
  analogWrite(rgb_BLUE, b);
}