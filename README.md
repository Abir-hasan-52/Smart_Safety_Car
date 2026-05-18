# 🚗 Smart Safety Car — Arduino Project

> An autonomous obstacle-avoiding and gas-detecting robotic car with Bluetooth remote control, multi-sensor fusion, and intelligent escape logic.

---

## 📌 Table of Contents

- [Overview](#overview)
- [Hardware Requirements](#hardware-requirements)
- [Pin Configuration](#pin-configuration)
- [Features](#features)
- [How to Use](#how-to-use)
- [Bluetooth Commands](#bluetooth-commands)
- [System Behavior Logic](#system-behavior-logic)
- [Bug Fixes Applied](#bug-fixes-applied)
- [Planned Upgrades](#planned-upgrades)
- [Known Limitations](#known-limitations)
- [Project Structure](#project-structure)

---

## Overview

The **Smart Safety Car** is an Arduino-based robotic vehicle designed for real-world safety scenarios. It combines multiple sensors — ultrasonic, IR, and gas — to autonomously detect and avoid obstacles, alert users to gas leaks, and accept manual Bluetooth commands from a smartphone app.

The car operates in two modes:
- **Autonomous Mode** — sensor-driven obstacle avoidance and alerts
- **Manual Override Mode** — full Bluetooth remote control

---

## Hardware Requirements

| Component | Quantity | Purpose |
|---|---|---|
| Arduino Uno / Nano | 1 | Main microcontroller |
| L298N Motor Driver | 1 | Controls 2 DC motors |
| DC Gear Motors | 2 | Drive the car |
| HC-SR04 Ultrasonic Sensor | 1 | Front distance measurement |
| IR Sensor Module | 1 | Close-range obstacle detection |
| MQ-2 / MQ-5 Gas Sensor | 1 | Gas / smoke detection |
| HC-05 / HC-06 Bluetooth Module | 1 | Wireless communication |
| Active Buzzer | 1 | Audio alerts |
| Red LED | 1 | Danger indicator |
| Green LED | 1 | Safe status indicator |
| 7.4V Li-ion Battery / Power Bank | 1 | Power supply |
| Jumper Wires + Chassis | — | Assembly |

---

## Pin Configuration

| Pin | Component | Direction |
|---|---|---|
| D2 (IN1) | Motor A — direction | OUTPUT |
| D3 (IN2) | Motor A — direction | OUTPUT |
| D4 (IN3) | Motor B — direction | OUTPUT |
| D5 (ENA) | Motor A — speed (PWM) | OUTPUT |
| D6 (ENB) | Motor B — speed (PWM) | OUTPUT |
| D7 (IN4) | Motor B — direction | OUTPUT |
| D8 (TRIG) | Ultrasonic — trigger | OUTPUT |
| D9 (ECHO) | Ultrasonic — echo | INPUT |
| D10 | Bluetooth RX | INPUT |
| D11 | Bluetooth TX | OUTPUT |
| D12 | Buzzer | OUTPUT |
| A0 | IR Sensor | INPUT (Digital) |
| A2 | Gas Sensor | INPUT (Analog) |
| A3 | Red LED | OUTPUT |
| A4 | Green LED | OUTPUT |

---

## Features

### 🔵 Bluetooth Remote Control
- Control the car from any Android phone using a Bluetooth terminal app (e.g., Serial Bluetooth Terminal)
- Adjustable speed levels: **Low**, **Medium**, **High**
- Toggle **Manual Override** to take full control

### 🟡 Ultrasonic Obstacle Detection
- Measures front distance using HC-SR04
- **Warning zone** (< 30 cm) — buzzer and LED blink in a non-blocking pattern
- **Danger zone** (< 15 cm) — triggers Auto Escape sequence
- Invalid readings (0 or out of range) are safely ignored and returned as `-1`

### 🔴 IR Sensor Detection
- Detects very close obstacles (reflective surface / proximity)
- Instantly triggers Auto Escape on detection

### ☠️ Gas / Smoke Alert
- Reads analog value from MQ-series gas sensor
- If value exceeds threshold (default: `400`), car stops immediately
- Non-blocking buzzer alert while gas is present
- **Automatically resumes** when gas level drops back to safe

### 🤖 Auto Escape Logic
1. **Stop** immediately
2. **Reverse** for 800ms
3. **Turn Left** — check if path is clear
4. If clear → move forward
5. If blocked → reverse again, **Turn Right**
6. If clear → move forward
7. If still blocked → **Stuck Alert** and request manual control

### 💡 LED Status System

| LED State | Meaning |
|---|---|
| Green ON | Safe to move |
| Red ON | Obstacle / Gas / Stuck |
| Red Blinking | Warning zone (15–30 cm) |

### 🔊 Buzzer Alert Patterns

| Pattern | Meaning |
|---|---|
| 3 short beeps | Escape sequence started |
| Rapid beeping | Gas detected |
| 3 long beeps | Car stuck — manual help needed |
| Slow blink | Warning zone |

---

## How to Use

### Step 1 — Upload Code
1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Open `smart_safety_car_fixed.ino`
3. Connect Arduino via USB
4. Select the correct **Board** and **Port** under Tools
5. Click **Upload**

### Step 2 — Connect Bluetooth
1. Power on the car
2. Pair your phone with **HC-05** or **HC-06** (default PIN: `1234` or `0000`)
3. Open a Bluetooth terminal app on your phone
4. Connect to the module — you should see `"Smart Safety Car Ready!"`

### Step 3 — Control the Car
Send single-character commands via the Bluetooth terminal (see table below).

### Step 4 — Gas Sensor Warm-Up
Mount the MQ sensor facing forward or downward for best detection. Allow **2–3 minutes warm-up time** after powering on for accurate readings before driving.

---

## Bluetooth Commands

| Command | Action |
|---|---|
| `F` | Move Forward |
| `B` | Move Backward |
| `L` | Turn Left |
| `R` | Turn Right |
| `S` | Stop |
| `H` | Speed → High (PWM 220) |
| `M` | Speed → Medium (PWM 150) |
| `L` | Speed → Low (PWM 100) |
| `X` | Toggle Manual Override ON / OFF |

> **Note:** In **Autonomous Mode** (Override OFF), the car ignores movement commands when an obstacle or gas is detected. In **Manual Override Mode**, all sensor-based stops are disabled.

---

## System Behavior Logic

```
Power ON
    │
    ▼
Read BT Command ──► handleBluetooth()
    │
    ▼
Read Sensors (Ultrasonic / IR / Gas)
    │
    ├── Gas > 400? ─────────► Stop + Gas Alert (non-blocking)
    │                              └── Gas cleared? → Resume + Safe Mode
    │
    ├── IR = LOW? ──────────► Auto Escape Sequence
    │
    ├── Distance < 15cm? ──► Auto Escape Sequence
    │
    ├── Distance < 30cm? ──► Warning Alert (non-blocking blink)
    │
    └── All clear? ─────────► Safe Mode (Green LED ON)
                                    │
                                    ▼
                          Execute BT Movement Command
```

---

## Bug Fixes Applied

| # | Original Issue | Fix Applied |
|---|---|---|
| 1 | `warningAlert()` used `delay(50)` which blocked the entire main loop | Replaced with `millis()`-based non-blocking LED and buzzer toggle every 100ms |
| 2 | Gas alert permanently stopped the car with no way to resume | Added `gasPreviouslyDetected` flag — car resumes automatically when gas level clears |
| 3 | `btCommand = 'F'` set inside auto escape conflicted when Manual Override was ON | Added `if (!manualOverride)` guard before setting `btCommand` in escape logic |
| 4 | Ultrasonic `pulseIn()` returning `0` was passed into calculations as a valid distance | Function now returns `-1` for any invalid reading; all decision points check `distance != -1` before acting |

---

## Planned Upgrades

### 🔧 Hardware Upgrades

| Upgrade | Benefit |
|---|---|
| Servo motor on Ultrasonic | 180° scan — smarter path selection |
| Side IR / Ultrasonic sensors | Detect left and right obstacles before turning |
| DHT11 Temperature Sensor | Fire and heat detection alongside gas |
| OLED Display (I2C, 0.96") | Show live distance, gas value, speed on the car |
| Encoder on motors | Precise distance and speed measurement |
| LiPo battery with voltage monitor | Know when battery is low via Bluetooth alert |

### 💻 Software Upgrades

| Upgrade | Benefit |
|---|---|
| Replace all `delay()` with `millis()` | True non-blocking operation throughout |
| PID Motor Control | Smooth, drift-free turning and straight driving |
| Speed auto-adjust by distance | Slow near obstacles, full speed in open space |
| Set gas threshold via Bluetooth | Tune sensitivity without re-uploading code |
| Servo scan before turning | Check best direction before committing to escape turn |
| Watchdog Timer | Auto-reset Arduino if the code ever freezes |

---

## Known Limitations

- Ultrasonic sensor does not detect transparent or soft/sound-absorbing surfaces reliably
- Gas sensor requires 2–3 minutes of warm-up time after power-on for accurate analog values
- Auto escape uses fixed timing delays — may not behave perfectly on all surface types or inclines
- No left or right obstacle sensing — the escape direction chosen is not always the optimal one
- Bluetooth range is limited to approximately 10 meters with HC-05 / HC-06
- `escapeBeep()` and `stuckAlert()` still use `delay()` internally — acceptable since they only run during a full stop, but can be improved in a future version

---

## Project Structure

```
smart-safety-car/
│
├── smart_safety_car_fixed.ino    # Main Arduino source code (fixed version)
├── README.md                     # Project documentation (this file)
│
└── docs/
    ├── circuit_diagram.png       # Wiring diagram (add manually)
    └── demo_video.mp4            # Demo footage (add manually)
```

---

## Author Notes

This project was built to demonstrate real-world sensor fusion on a low-cost Arduino platform. The goal was not just a moving robot, but a **safety-aware** vehicle that reacts intelligently to its environment — stopping for gas, escaping obstacles, and resuming automatically when danger clears.

All four critical bugs from the original version have been identified and resolved. The codebase is now non-blocking, conflict-free, and ready for hardware extension.

---

*Built with Arduino UNO · HC-SR04 Ultrasonic · MQ Gas Sensor · HC-05 Bluetooth · L298N Motor Driver*