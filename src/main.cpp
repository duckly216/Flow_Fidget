#include <Arduino.h>
#include <math.h>
// ── Device States ─────────────────────────────────────────────────────────────────────
#define STATE_OFF 0
#define STATE_FIDGET 1
#define STATE_SCROLL 2

// ── Timer States ─────────────────────────────────────────────────────────────────────
#define TIMER_IDLE  0
#define TIMER_WORK  1
#define TIMER_BREAK 2
#define TIMER_ALERT 3

// ── Timing ─────────────────────────────────────────────────────────────────────
#define WORK_DURATION       (25UL * 60UL * 1000UL)  // 25 minutes
#define SHORT_BREAK_DURATION (5UL * 60UL * 1000UL)  //  5 minutes (after sessions 1–3)
#define LONG_BREAK_DURATION (15UL * 60UL * 1000UL)  // 15 minutes (after session 4)
#define ALERT_DURATION      (10UL * 1000UL)          // 10 s alert flash

#define DEBOUNCE_MS   50
#define LONG_PRESS_MS 1000

// ── Pins ───────────────────────────────────────────────────────────────────────
const int BUTTON_PIN = 3;   // mode/power button  (INPUT_PULLUP)
const int STICK_BTN  = 2;   // joystick click     (INPUT_PULLUP, unused here)

const int RGB_R = 9;
const int RGB_G = 10;
const int RGB_B = 6;

const int LED_RED    = 11;  // pomodoro 1
const int LED_YELLOW = 12;  // pomodoro 2
const int LED_GREEN  = 13;  // pomodoro 3

// ── State ──────────────────────────────────────────────────────────────────────
int           currentState  = TIMER_IDLE;
int           nextState     = TIMER_WORK;
unsigned long sessionStart  = 0;
unsigned long sessionLen    = 0;
unsigned long alertStart    = 0;
int           pomodoroCount = 0;  // 0–4: sessions completed in current set
unsigned long pressStartTime = 0;
bool isHolding = false;

// Button debounce
bool          lastBtnRaw    = HIGH;
unsigned long lastDebounce  = 0;
unsigned long btnPressTime  = 0;
bool          btnHandled    = false;

// ── Helpers ────────────────────────────────────────────────────────────────────
void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(RGB_R, r);
  analogWrite(RGB_G, g);
  analogWrite(RGB_B, b);
}

void clearAll() {
  setRGB(0, 0, 0);
  digitalWrite(LED_RED,    LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_GREEN,  LOW);
}

// LEDs show completed sessions: 1 = red, 2 = +yellow, 3 = +green, 4 = all on
void showCount() {
  digitalWrite(LED_RED,    pomodoroCount >= 1 ? HIGH : LOW);
  digitalWrite(LED_YELLOW, pomodoroCount >= 2 ? HIGH : LOW);
  digitalWrite(LED_GREEN,  pomodoroCount >= 3 ? HIGH : LOW);
  // All three lit = 4th session done (long break coming)
}

void printTime(const char* label, unsigned long remainingMs) {
  unsigned int m = remainingMs / 60000UL;
  unsigned int s = (remainingMs % 60000UL) / 1000UL;
  Serial.print(label);
  if (m < 10) Serial.print('0');
  Serial.print(m);
  Serial.print(':');
  if (s < 10) Serial.print('0');
  Serial.println(s);
}

// ── State Transitions ──────────────────────────────────────────────────────────
void enterIdle() {
  currentState = TIMER_IDLE;
  clearAll();
  showCount();
  Serial.println(F("[IDLE] press button to start"));
}

void enterWork() {
  currentState = TIMER_WORK;
  sessionStart = millis();
  sessionLen   = WORK_DURATION;
  setRGB(255, 0, 0);  // red = focus
  showCount();
  Serial.println(F("[WORK] 25:00 started"));
}

void enterBreak(unsigned long duration) {
  currentState = TIMER_BREAK;
  sessionStart = millis();
  sessionLen   = duration;
  setRGB(0, 200, 0);  // green = rest
  showCount();
  if (duration == LONG_BREAK_DURATION) {
    Serial.println(F("[LONG BREAK] 15:00 started"));
  } else {
    Serial.println(F("[BREAK] 5:00 started"));
  }
}

void enterAlert(int after) {
  currentState = TIMER_ALERT;
  nextState    = after;
  alertStart   = millis();
  Serial.println(F("[ALERT] time's up!"));
}

void resetAll() {
  pomodoroCount = 0;
  nextState     = TIMER_WORK;
  enterIdle();
  Serial.println(F("[RESET]"));
}

// ── Setup ──────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(STICK_BTN,  INPUT_PULLUP);

  pinMode(RGB_R,     OUTPUT);
  pinMode(RGB_G,     OUTPUT);
  pinMode(RGB_B,     OUTPUT);
  pinMode(LED_RED,    OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN,  OUTPUT);

  enterIdle();
}

// ── Loop ───────────────────────────────────────────────────────────────────────
void loop() {
  int BUTTON_VALUE = digitalRead(BUTTON_PIN);
  switch (currentState) {
    case STATE_OFF:
      // Button is being pressed and it was NOT being pressed originally
      // Serial.println("Gadget currently off");

      if(BUTTON_VALUE == LOW && !isHolding){
        pressStartTime = millis();
        isHolding = true;
        // Serial.println("Button Pressing Initialized");
      }
      if(BUTTON_VALUE == LOW && isHolding){
        unsigned long duration = millis() - pressStartTime;
        // At 1 seconds, turn on the Yellow LED
        if (duration > 1000) {digitalWrite(LED_YELLOW, HIGH);}
        
        // At 3 seconds, turn on the Green LED
        if (duration > 3000) {digitalWrite(LED_GREEN, HIGH);}
        // At 5 seconds, Power On
        if (duration >= 5000) {
            currentState = STATE_SCROLL;
            isHolding = false;
            digitalWrite(LED_YELLOW, LOW);
            
            digitalWrite(LED_GREEN, LOW); delay(200);
            digitalWrite(LED_GREEN, HIGH); delay(200);
            digitalWrite(LED_GREEN, LOW); delay(200);
            digitalWrite(LED_GREEN, HIGH); delay(200); 
            digitalWrite(LED_GREEN, LOW);
            // Turn everything off or flash green to signal "Ready"
        }
      }
      if(BUTTON_VALUE == HIGH){
        isHolding = false;
        clearAll();
      }
      break;
    case STATE_FIDGET:
      setRGB(0, 0, 255);
    break;
    case STATE_SCROLL:
      setRGB(135, 61, 22);
      
    break;
  } 

}

