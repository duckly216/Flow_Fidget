# Flow_Fidget

Flow_Fidget is an Arduino + PlatformIO project for a joystick-driven fidget and scroll controller.

Right now, the project is implemented as a **Pomodoro timer** with LED feedback, button controls, and a simple state machine. The long-term goal is to expand it into a joystick-based physical productivity/fidget device that can also support scrolling behavior.

## Features

- 4-state Pomodoro workflow
- RGB LED feedback for current timer state
- 3 individual LEDs to track completed work sessions
- Short press and long press button controls
- Non-blocking timing using `millis()`
- Serial output for debugging and monitoring
- Built with **PlatformIO** for Arduino Uno

---

## Current States

| State | Description |
|---|---|
| `STATE_IDLE` | Waiting state with a blue breathing RGB LED |
| `STATE_WORK` | 25-minute focus session with solid red RGB LED |
| `STATE_BREAK` | 5-minute short break, or 15-minute long break after session 4, with solid green RGB LED |
| `STATE_ALERT` | Session ended, all LEDs flash white for 10 seconds |

A full Pomodoro set consists of **4 work sessions**:

- Sessions 1 to 3 are followed by a **5-minute short break**
- Session 4 is followed by a **15-minute long break**
- After the long break, the session count resets and the cycle continues

---

## Hardware

### Inputs

- **Joystick X-axis:** `A0`
- **Joystick Y-axis:** `A1`
- **Joystick button:** `D2` (`INPUT_PULLUP`)
- **Mode/Power button:** `D3` (`INPUT_PULLUP`)

### Individual LED Outputs

- **Red LED:** `D11`
- **Yellow LED:** `D12`
- **Green LED:** `D13`

### RGB LED Outputs

- **RGB Red:** `D6`
- **RGB Green:** `D10`
- **RGB Blue:** `D9`

> Note: `scrollFunctionality.cpp` is a separate joystick serial test sketch and is not compiled by default in PlatformIO.

---

## Project Structure

```text
Flow_Fidget/
├── src/
│   └── main.cpp                 # Main application entry point and Pomodoro state machine
├── scrollFunctionality.cpp      # Standalone joystick read/debug example
├── platformio.ini               # PlatformIO configuration for Arduino Uno
└── README.md

