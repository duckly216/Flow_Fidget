#include <Arduino.h>
#include <math.h>

// ── Device States ───────────────────────────────────────────────────────────
#define STATE_OFF    0
#define STATE_FIDGET 1
#define STATE_SCROLL 2

// ── Pins ────────────────────────────────────────────────────────────────────
const int BUTTON_PIN = 3;
const int STICK_BTN  = 2;

const int JOY_X = A0;
const int JOY_Y = A1;

const int RGB_R = 9;
const int RGB_G = 10;
const int RGB_B = 6;

const int LED_RED    = 11;
const int LED_YELLOW = 12;
const int LED_GREEN  = 13;

const int BUZZER_PIN = 8;

// ── Scroll Config ───────────────────────────────────────────────────────────
const int  JOY_CENTER  = 512;
const int  DEAD_ZONE   = 60;
const int  SCROLL_MAX  = 10;
const unsigned long SCROLL_INTERVAL_MS = 50;

unsigned long lastScrollTick    = 0;

// ── Device State ────────────────────────────────────────────────────────────
int           currentState     = STATE_OFF;

// ── Inactivity Tracking ─────────────────────────────────────────────────────
unsigned long lastActivityTime  = 0;

// ── Button State ────────────────────────────────────────────────────────────
unsigned long pressStartTime   = 0;
unsigned long lastReleaseTime  = 0;
int           clickCount       = 0;
bool          isHolding        = false;

// ── Helpers ─────────────────────────────────────────────────────────────────
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

void showModeColor(int state) {
  switch (state) {
    case STATE_FIDGET:
      setRGB(135, 61, 22);
      break;

    case STATE_SCROLL:
      setRGB(0, 0, 255);
      break;

    default:
      setRGB(0, 0, 0);
      break;
  }
}

void printModeBanner(int state) {
  switch (state) {
    case STATE_OFF:
      Serial.println(F("MODE:OFF"));
      break;

    case STATE_FIDGET:
      Serial.println(F("MODE:FIDGET"));
      break;

    case STATE_SCROLL:
      Serial.println(F("MODE:SCROLL"));
      break;
  }
}

// ── Inactivity Detection ────────────────────────────────────────────────────
bool checkActivity() {
  int rawX = analogRead(JOY_X);
  int rawY = analogRead(JOY_Y);
  bool joyActive = (abs(rawX - JOY_CENTER) > DEAD_ZONE) ||
                   (abs(rawY - JOY_CENTER) > DEAD_ZONE);
  bool btnActive = (digitalRead(BUTTON_PIN) == LOW) ||
                   (digitalRead(STICK_BTN) == LOW);

  if (joyActive || btnActive) {
    lastActivityTime = millis();
    return true;
  }
  return false;
}

void updateInactivityLEDs() {
  unsigned long idle = millis() - lastActivityTime;

  digitalWrite(LED_GREEN,  LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED,    LOW);
  noTone(BUZZER_PIN);

  if (idle < 20000UL) {
    digitalWrite(LED_GREEN, HIGH);
  } else if (idle < 40000UL) {
    digitalWrite(LED_YELLOW, HIGH);
  } else if (idle < 60000UL) {
    digitalWrite(LED_YELLOW, (millis() % 500 < 250) ? HIGH : LOW);
  } else {
    digitalWrite(LED_RED, HIGH);
    tone(BUZZER_PIN, 1000);
  }
}

// ── Device State Transitions ────────────────────────────────────────────────
void transitionTo(int newState) {
  currentState = newState;

  switch (newState) {
    case STATE_OFF:
      Serial.println(F("System: Powered Off"));
      noTone(BUZZER_PIN);
      clearAll();
      printModeBanner(newState);
      break;

    case STATE_FIDGET:
      Serial.println(F("System: Fidget Mode"));
      lastActivityTime = millis();
      showModeColor(newState);
      printModeBanner(newState);
      break;

    case STATE_SCROLL:
      Serial.println(F("System: Scroll Mode"));
      lastActivityTime = millis();
      showModeColor(newState);
      printModeBanner(newState);
      break;
  }
}

// ── Button Interaction ──────────────────────────────────────────────────────
void buttonInteraction() {
  int btnVal = digitalRead(BUTTON_PIN);

  // Which LED lights up during long-press, and what state do we go to?
  int focusLed      = LED_GREEN;
  int longPressGoal = STATE_SCROLL;

  if (currentState == STATE_FIDGET || currentState == STATE_SCROLL) {
    focusLed      = LED_RED;
    longPressGoal = STATE_OFF;
  }

  // ── Long-press detection ──────────────────────────────────────────────────
  if (btnVal == LOW && !isHolding) {
    pressStartTime = millis();
    isHolding = true;
  }

  if (btnVal == LOW && isHolding) {
    unsigned long duration = millis() - pressStartTime;

    if (duration > 1000) { digitalWrite(LED_YELLOW, HIGH); }
    if (duration > 3000) { digitalWrite(focusLed, HIGH); }

    if (duration >= 5000) {
      transitionTo(longPressGoal);
      isHolding = false;
      digitalWrite(LED_YELLOW, LOW);

      // Confirmation blink
      digitalWrite(focusLed, LOW);  delay(200);
      digitalWrite(focusLed, HIGH); delay(200);
      digitalWrite(focusLed, LOW);  delay(200);
      digitalWrite(focusLed, HIGH); delay(200);
      digitalWrite(focusLed, LOW);

      showModeColor(currentState);
    }
  }

  // ── Short-tap detection (on release) ──────────────────────────────────────
  if (btnVal == HIGH && isHolding) {
    unsigned long duration = millis() - pressStartTime;
    isHolding = false;

    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(focusLed,   LOW);

    if (duration < 500 && currentState != STATE_OFF) {
      clickCount++;
      lastReleaseTime = millis();
    }
  }

  // ── Double-click → toggle FIDGET ↔ SCROLL ────────────────────────────────
  if (clickCount > 0 && (millis() - lastReleaseTime > 400)) {
    if (clickCount == 2) {
      if (currentState == STATE_FIDGET) {
        transitionTo(STATE_SCROLL);
      } else if (currentState == STATE_SCROLL) {
        transitionTo(STATE_FIDGET);
      }
    }
    clickCount = 0;
  }

  // Keep everything off while device is off (and not mid-press)
  if (currentState == STATE_OFF && !isHolding) {
    setRGB(0, 0, 0);
    clearAll();
  }
}

// ── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(STICK_BTN,  INPUT_PULLUP);

  pinMode(RGB_R,      OUTPUT);
  pinMode(RGB_G,      OUTPUT);
  pinMode(RGB_B,      OUTPUT);
  pinMode(LED_RED,    OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN,  OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.println(F("FLOW_FIDGET_READY"));
  transitionTo(STATE_OFF);
  Serial.println(F("[OFF] long-press to start"));
}

// ── Loop ────────────────────────────────────────────────────────────────────
void loop() {
  buttonInteraction();

  switch (currentState) {
    case STATE_OFF:
      break;

    case STATE_FIDGET:
      checkActivity();
      updateInactivityLEDs();
      break;

    case STATE_SCROLL: {
      checkActivity();
      updateInactivityLEDs();

      int rawY   = analogRead(JOY_Y);
      int offset = rawY - JOY_CENTER;

      if (abs(offset) > DEAD_ZONE && millis() - lastScrollTick >= SCROLL_INTERVAL_MS) {
        lastScrollTick = millis();
        int direction  = (offset > 0) ? 1 : -1;
        int magnitude  = abs(offset) - DEAD_ZONE;
        int speed      = map(magnitude, 0, JOY_CENTER - DEAD_ZONE, 1, SCROLL_MAX);
        speed = constrain(speed, 1, SCROLL_MAX);

        Serial.print(F("SCROLL:"));
        Serial.print(direction * speed);
        Serial.print(F(":"));
        Serial.println(speed);
      }
      break;
    }
  }
}
