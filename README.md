# 🎮 ESP32 Breakout — OLED Brick Breaker with Potentiometer Control

A fully playable, single-file **Breakout / Brick Breaker** arcade game running on an **ESP32** microcontroller with a 128×64 SSD1306 OLED display and a potentiometer-controlled paddle. No buttons, no touchscreen — just a knob and a tiny screen.

---

## 📸 Overview

This project brings the classic Breakout arcade experience to embedded hardware using minimal components. The ball bounces, bricks break, lives are tracked, and difficulty scales as you clear levels. Everything — game logic, rendering, and input — runs in a tight loop with frame timing kept to ~12 ms.

---

## 🔧 Hardware Requirements

| Component | Details |
|---|---|
| Microcontroller | ESP32 (any variant with I²C and ADC) |
| Display | SSD1306 128×64 OLED (I²C, address `0x3C` or `0x3D`) |
| Input | 10kΩ potentiometer (linear taper recommended) |
| Wiring | SDA → GPIO 4, SCL → GPIO 5, POT → GPIO 0 (ADC) |
| Power | USB or 3.3 V regulated supply |

> **Note:** GPIO 0 is the ADC input for the potentiometer. On some ESP32 boards, GPIO 0 is also the boot button — if you see erratic ADC readings at startup, add a small RC filter or remap to another ADC-capable pin (e.g., GPIO 34–39).

---

## 📦 Dependencies

This project uses the **Arduino framework** for ESP32. Install the following libraries via the Arduino Library Manager or PlatformIO:

| Library | Purpose |
|---|---|
| [U8g2](https://github.com/olikraus/u8g2) | OLED rendering (full-frame buffer mode) |
| Wire (built-in) | I²C communication |

No other external libraries are needed.

---

## 🚀 Getting Started

### 1. Clone the repository

```bash
git clone https://github.com/yourusername/esp32-breakout.git
cd esp32-breakout
```

### 2. Open in Arduino IDE or PlatformIO

- **Arduino IDE:** Open `breakout.ino`, select your ESP32 board under *Tools → Board*, and select the correct COM port.
- **PlatformIO:** Add `lib_deps = olikraus/U8g2` to your `platformio.ini` and build.

### 3. Wire up the hardware

```
ESP32 GPIO 4  →  OLED SDA
ESP32 GPIO 5  →  OLED SCL
ESP32 3.3V    →  OLED VCC
ESP32 GND     →  OLED GND

ESP32 GPIO 0  →  Potentiometer wiper (middle pin)
ESP32 3.3V    →  Potentiometer one end
ESP32 GND     →  Potentiometer other end
```

### 4. Flash and play

Upload the sketch. The title screen appears immediately. Rotate the pot all the way to the **right** and hold for ~0.6 seconds to start the game.

---

## 🎮 Gameplay

### Controls

| Action | Input |
|---|---|
| Move paddle left/right | Rotate the potentiometer |
| Start game / restart | Hold pot at full-right (~4000 ADC) for 0.6 s |

### Rules

- **Break all bricks** in a row to advance to the next level.
- You have **3 lives**. Missing the ball costs one life.
- Each brick is worth **5 points**.
- The paddle **shrinks by 2 px** each time you clear the board, making later levels harder.
- Ball speed increases subtly through **spin physics** — hitting the paddle off-center adds horizontal velocity.

### HUD

```
S:0  L:3
────────────────────────────
```

Score and remaining lives are displayed at the top. A horizontal rule separates the HUD from the play field.

---

## 🧠 Technical Notes

### Input Smoothing

Raw ADC readings from the potentiometer are processed through an **Exponential Moving Average (EMA)** filter with α = 0.10. This eliminates jitter without adding perceptible input lag.

```c
potEMA = (1 - ALPHA) * potEMA + ALPHA * rawADC;
```

### Ball Physics

The ball uses floating-point velocity (`ballVX`, `ballVY`). When the ball strikes the paddle, an **offset-based spin** mechanic is applied:

```c
float offset = (ballX - (padX + padW / 2.0f)) / (padW / 2.0f); // -1.0 to 1.0
ballVX += offset * 0.8f;
ballVX = clamp(ballVX, -3.2f, 3.2f);
```

This gives skilled players control over the ball's angle, rewarding deliberate paddle positioning.

### Brick Collision

Collision detection uses a simple **AABB point-in-rectangle** test (`hitRect`). When a hit is detected, the vertical velocity is reversed and the loop exits via `goto bricksDone` to prevent multiple brick hits per frame.

### Rendering

U8g2 is used in **full-frame buffer mode** (`_F_`), meaning the entire 128×64 frame is built in RAM and flushed to the display in one shot per frame. This avoids flickering artifacts common with partial-update approaches.

Frame timing targets ~12 ms per loop iteration (`delay(12)`), giving approximately **83 FPS** theoretical throughput — well within the OLED's refresh capability.

### Start Gesture

To avoid accidental game starts, a **hold gesture** is required: the potentiometer must stay above ADC value 4000 for at least **600 ms** continuously. This same gesture also restarts from the Game Over screen.

---

## 📁 File Structure

```
esp32-breakout/
├── breakout.ino       # Complete game — all logic in one file
└── README.md
```

This is intentionally a single-file project for simplicity and portability across Arduino and PlatformIO setups.

---

## ⚙️ Configuration Reference

| Constant | Default | Description |
|---|---|---|
| `SDA_PIN` | 4 | I²C data pin |
| `SCL_PIN` | 5 | I²C clock pin |
| `OLED_ADDR_8BIT` | `0x3C << 1` | OLED I²C address (try `0x3D` if blank) |
| `POT_PIN` | 0 | ADC pin for potentiometer |
| `POT_ALPHA` | 0.10 | EMA smoothing factor (lower = smoother, more lag) |
| `brickRows` | 4 | Number of brick rows |
| `brickCols` | 10 | Number of brick columns |
| `START_THRESHOLD` | 4000 | ADC value for start gesture |
| `START_HOLD_MS` | 600 | Hold duration (ms) to trigger start |
| `padW` (initial) | 26 px | Starting paddle width |

---

## 🐛 Troubleshooting

**OLED is blank after flashing**
Try changing `OLED_ADDR_8BIT` from `(0x3C << 1)` to `(0x3D << 1)`. Some SSD1306 clones ship with the alternate address.

**Pot input is erratic or inverted**
Check that the potentiometer ends are connected to 3.3V and GND (not 5V — the ESP32 ADC is not 5V tolerant). If values are inverted, swap the power/ground leads.

**Game starts immediately without holding**
Your ADC may be reading near 4095 at rest. Either remap `START_THRESHOLD` higher, or physically reposition the potentiometer's rest position.

**Ball clips through bricks at high speed**
The collision detection uses a single-point check per frame. At very high `ballVX`/`ballVY` values the ball can tunnel. The velocity cap of `±3.2` prevents this in normal gameplay.

---

## 🗺️ Roadmap / Ideas

- [ ] High score persistence using ESP32 NVS (non-volatile storage)
- [ ] Multiple brick hit points (stronger bricks requiring 2–3 hits)
- [ ] Power-ups: wide paddle, multi-ball, slow motion
- [ ] Sound effects via a passive buzzer on a PWM pin
- [ ] Difficulty select on the title screen
- [ ] Wi-Fi leaderboard using ESP32's built-in wireless

---

## 📄 License

MIT License. See `LICENSE` for details. Free to use, modify, and distribute.

---

## 🙏 Acknowledgements

- [U8g2 library](https://github.com/olikraus/u8g2) by Oliver Kraus — an outstanding embedded graphics library that makes working with tiny displays genuinely enjoyable.
- The original **Breakout** (1976) by Atari, designed by Steve Wozniak and Steve Jobs — still endlessly fun nearly 50 years later.
