#include <Arduino.h>
#include <math.h>

// ── States ─────────────────────────────────────────────────────────────────────
#define STATE_IDLE  0
#define STATE_WORK  1
#define STATE_BREAK 2
#define STATE_ALERT 3

// ── Timing ─────────────────────────────────────────────────────────────────────
#define WORK_DURATION       (25UL * 60UL * 1000UL)  // 25 minutes
#define SHORT_BREAK_DURATION( 5UL * 60UL * 1000UL)  //  5 minutes (after sessions 1–3)
#define LONG_BREAK_DURATION (15UL * 60UL * 1000UL)  // 15 minutes (after session 4)
#define ALERT_DURATION      (10UL * 1000UL)          // 10 s alert flash

#define DEBOUNCE_MS   50
#define LONG_PRESS_MS 1000

// ── Pins ───────────────────────────────────────────────────────────────────────
const int BUTTON_PIN = 3;   // mode/power button  (INPUT_PULLUP)
const int STICK_BTN  = 2;   // joystick click     (INPUT_PULLUP, unused here)

const int RGB_R = 6;
const int RGB_G = 10;
const int RGB_B = 9;

const int LED_RED    = 11;  // pomodoro 1
const int LED_YELLOW = 12;  // pomodoro 2
const int LED_GREEN  = 13;  // pomodoro 3

// ── State ──────────────────────────────────────────────────────────────────────
int           currentState  = STATE_IDLE;
int           nextState     = STATE_WORK;
unsigned long sessionStart  = 0;
unsigned long sessionLen    = 0;
unsigned long alertStart    = 0;
int           pomodoroCount = 0;  // 0–4: sessions completed in current set

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
  currentState = STATE_IDLE;
  clearAll();
  showCount();
  Serial.println(F("[IDLE] press button to start"));
}

void enterWork() {
  currentState = STATE_WORK;
  sessionStart = millis();
  sessionLen   = WORK_DURATION;
  setRGB(255, 0, 0);  // red = focus
  showCount();
  Serial.println(F("[WORK] 25:00 started"));
}

void enterBreak(unsigned long duration) {
  currentState = STATE_BREAK;
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
  currentState = STATE_ALERT;
  nextState    = after;
  alertStart   = millis();
  Serial.println(F("[ALERT] time's up!"));
}

void resetAll() {
  pomodoroCount = 0;
  nextState     = STATE_WORK;
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
  unsigned long now = millis();

  // ── Button: debounce + short/long press ─────────────────────────────────────
  bool btnRaw = digitalRead(BUTTON_PIN);

  if (btnRaw != lastBtnRaw) lastDebounce = now;

  if ((now - lastDebounce) > DEBOUNCE_MS) {
    // Falling edge: button just pressed
    if (btnRaw == LOW && lastBtnRaw == HIGH) {
      btnPressTime = now;
      btnHandled   = false;
    }

    // Still held: check for long press (reset)
    if (btnRaw == LOW && !btnHandled) {
      if ((now - btnPressTime) >= LONG_PRESS_MS) {
        btnHandled = true;
        resetAll();
      }
    }

    // Rising edge: short press (start / acknowledge)
    if (btnRaw == HIGH && lastBtnRaw == LOW && !btnHandled) {
      btnHandled = true;
      if (currentState == STATE_IDLE) {
        enterWork();
      }
    }
  }
  lastBtnRaw = btnRaw;

  // ── State Machine ────────────────────────────────────────────────────────────
  switch (currentState) {

    case STATE_IDLE: {
      // Slow blue breathing effect
      uint8_t breath = (uint8_t)(128 + 127 * sin(now / 1200.0));
      setRGB(0, 0, breath);
      break;
    }

    case STATE_WORK:
    case STATE_BREAK: {
      unsigned long elapsed   = now - sessionStart;
      unsigned long remaining = (elapsed < sessionLen) ? (sessionLen - elapsed) : 0;

      // Serial countdown every second
      static unsigned long lastPrint = 0;
      if (now - lastPrint >= 1000UL) {
        lastPrint = now;
        printTime(currentState == STATE_WORK ? "WORK  " : "BREAK ", remaining);
      }

      if (elapsed >= sessionLen) {
        if (currentState == STATE_WORK) {
          pomodoroCount++;
          enterAlert(STATE_BREAK);  // always go to break next; duration decided in alert handler
        } else {
          // Break finished: always loop back into work
          if (pomodoroCount >= 4) pomodoroCount = 0;
          enterAlert(STATE_WORK);
        }
      }
      break;
    }

    case STATE_ALERT: {
      // Flash all lights white every 300 ms
      bool on = ((now - alertStart) / 300UL) % 2 == 0;
      setRGB(on ? 255 : 0, on ? 255 : 0, on ? 255 : 0);
      digitalWrite(LED_RED,    on ? HIGH : LOW);
      digitalWrite(LED_YELLOW, on ? HIGH : LOW);
      digitalWrite(LED_GREEN,  on ? HIGH : LOW);

      if ((now - alertStart) >= ALERT_DURATION) {
        if (nextState == STATE_WORK)       enterWork();
        else if (nextState == STATE_BREAK) enterBreak(pomodoroCount >= 4 ? LONG_BREAK_DURATION : SHORT_BREAK_DURATION);
        else                               enterIdle();
      }
      break;
    }
  }
}
