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

const int JOY_X = A0;
const int JOY_Y = A1;

const int RGB_R = 9;
const int RGB_G = 6;
const int RGB_B = 10;

const int LED_RED    = 11;  // pomodoro 1
const int LED_YELLOW = 12;  // pomodoro 2
const int LED_GREEN  = 13;  // pomodoro 3

const int BUZZER_PIN = 8;

// ── Scroll Config ───────────────────────────────────────────────────────────
const int  JOY_CENTER  = 512;
const int  DEAD_ZONE   = 60;
const int  SCROLL_MAX  = 10;
const unsigned long SCROLL_INTERVAL_MS = 50;
const unsigned long IDLE_YELLOW_MS = 20000UL;
const unsigned long IDLE_FLASH_MS  = 40000UL;
const unsigned long IDLE_ALARM_MS  = 60000UL;
const unsigned long FLASH_PERIOD_MS    = 500;       // on/off toggle for flash stage

unsigned long lastScrollTick  = 0;
unsigned long lastActivityTime = 0;

// ── State ──────────────────────────────────────────────────────────────────────
int           currentState  = STATE_OFF;
int           currentTimerState= TIMER_IDLE;
int           nextState     = TIMER_WORK;
unsigned long sessionStart  = 0;
unsigned long sessionLen    = 0;
unsigned long alertStart    = 0;
int           pomodoroCount = 0;  // 0–4: sessions completed in current set
unsigned long pressStartTime = 0;
unsigned long lastReleaseTime = 0;
int clickCount = 0;
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

// ── Timer State Transitions ──────────────────────────────────────────────────────────
void enterIdle() {
  currentTimerState = TIMER_IDLE;
  clearAll();
  showCount();
  Serial.println(F("[IDLE] press button to start"));
}

void enterWork() {
  currentTimerState = TIMER_WORK;
  sessionStart = millis();
  sessionLen   = WORK_DURATION;
  setRGB(255, 0, 0);  // red = focus
  showCount();
  Serial.println(F("[WORK] 25:00 started"));
}

// ── Gadget State Transitions ──────────────────────────────────────────────────────────
void transitionTo(int newState) {
    currentState = newState;

    switch (newState) {
        case STATE_OFF:
            Serial.println("System: Powered Off");
            setRGB(0, 0, 0); // Turn off RGB
            digitalWrite(LED_YELLOW, LOW);
            digitalWrite(LED_GREEN, LOW);
            break;

        case STATE_FIDGET:
            Serial.println("System: Fidget Mode"); 
            setRGB(0, 255, 0); // Green for Fidget
            break;

        case STATE_SCROLL:
            Serial.println("System: Scroll Mode");
            setRGB(0, 0, 255); // Blue for Scroll
            break;
    }
}
// ── Button Inputs ──────────────────────────────────────────────────────────
void buttonInteraction() {
  int BUTTON_VALUE = digitalRead(BUTTON_PIN);
  int FOCUS_LED = 0;
  int going_into_state = 0;
  // If device off, green light will turn on - indicates turning on
  // If device on, red light will turn on - indicates turning off
  if(currentState == STATE_OFF){
    FOCUS_LED = LED_GREEN;
    going_into_state = STATE_SCROLL;
  }
  if(currentState == STATE_FIDGET or currentState == STATE_SCROLL){
    FOCUS_LED = LED_RED;
    going_into_state = STATE_OFF;
  }


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
    if (duration > 3000) {
      digitalWrite(FOCUS_LED, HIGH);
    }
    // At 5 seconds, Power On/Off
    if (duration >= 5000) {
        currentState = going_into_state;
        if(currentState == STATE_OFF) Serial.println("System: Powered Off");
        if(currentState == STATE_FIDGET) Serial.println("System: Fidget Mode");
        if(currentState == STATE_SCROLL) Serial.println("System: Scroll Mode");
        isHolding = false;
        digitalWrite(LED_YELLOW, LOW);
        setRGB(0, 0, 0);
        
        digitalWrite(FOCUS_LED, LOW); delay(200);
        digitalWrite(FOCUS_LED, HIGH); delay(200);
        digitalWrite(FOCUS_LED, LOW); delay(200);
        digitalWrite(FOCUS_LED, HIGH); delay(200); 
        digitalWrite(FOCUS_LED, LOW);
        // Turn everything off or flash green to signal "Ready"
    }
  }
  // CLICKED SHORTLY
  if (BUTTON_VALUE == HIGH && isHolding) {
    unsigned long duration = millis() - pressStartTime;
    isHolding = false;

    // Reset indicator LEDs immediately on release
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(FOCUS_LED, LOW);

    // If it was a short tap (< 500ms) and the device is ON
    if (duration < 500 && currentState != STATE_OFF) {
        clickCount++;
        lastReleaseTime = millis();
    }
  }
  if (clickCount > 0 && (millis() - lastReleaseTime > 1000)) { // 1000ms window
    if (clickCount == 3) {
        // Toggle Logic
        Serial.println("change state");
        if (currentState == STATE_FIDGET) transitionTo(STATE_SCROLL);
        else transitionTo(STATE_FIDGET);
    }
    clickCount = 0; // Reset counter
  }
  
  // Only force everything off if we are in STATE_OFF AND the user isn't holding the button
  if (currentState == STATE_OFF && !isHolding) {
          setRGB(0, 0, 0);
      }

}

void enterBreak(unsigned long duration) {
  currentTimerState = TIMER_BREAK;
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
  currentTimerState = TIMER_ALERT;
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
  pinMode(BUZZER_PIN, OUTPUT);

  enterIdle();
}

// ── Loop ───────────────────────────────────────────────────────────────────────
void loop() {
  int BUTTON_VALUE = digitalRead(BUTTON_PIN);
  switch (currentState) {
    case STATE_OFF:
      buttonInteraction();
      break;
    case STATE_FIDGET:
      buttonInteraction();
      break;
    case STATE_SCROLL: {
      buttonInteraction();   
      int rawY = analogRead(JOY_Y);
      int offset = rawY - JOY_CENTER;
      bool active = abs(offset) > DEAD_ZONE;

      if (active) {
        lastActivityTime = millis();
        noTone(BUZZER_PIN);
      }

      if (active && millis() - lastScrollTick >= SCROLL_INTERVAL_MS) {
        lastScrollTick = millis();
        int direction = (offset > 0) ? 1 : -1;
        int magnitude = abs(offset) - DEAD_ZONE;
        int speed = map(magnitude, 0, JOY_CENTER - DEAD_ZONE, 1, SCROLL_MAX);
        speed = constrain(speed, 1, SCROLL_MAX);

        Serial.print(F("SCROLL:"));
        Serial.print(direction * speed);
        Serial.print(F(":"));
        Serial.println(speed);
      }

      unsigned long idleTime = millis() - lastActivityTime;

      if (idleTime < IDLE_YELLOW_MS) {
        setRGB(0, 255, 0);
      } else if (idleTime < IDLE_FLASH_MS) {
        setRGB(255, 200, 0);
      } else if (idleTime < IDLE_ALARM_MS) {
        bool flashOn = (millis() / FLASH_PERIOD_MS) % 2 == 0;
        if (flashOn) {
          setRGB(255, 200, 0);
        } else {
          setRGB(0, 0, 0);
        }
      } else {
        setRGB(255, 0, 0);
        tone(BUZZER_PIN, 1000);
      }
      break;
    }

  } 

}

