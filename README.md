# Smart Environment Monitoring System

An ESP32-based system that monitors temperature, humidity and sound, and shows the readings on a 0.96-inch OLED display. The user switches between two display pages using capacitive touch.

Developed for **A261 STTHK3113 – Sensor-Based System, Assignment 2**.

## Features

- Measures **temperature (°C)** and **humidity (%)** with a DHT11 sensor
- Detects sound with a KY-037 sound sensor and shows a **relative sound level (0–100 %)** and a status (**Sound Detected** / **Quiet**)
- Two OLED display pages: **Environment** and **Sound Monitor**
- **Capacitive touch** navigation using a male pin on GPIO 27, with debounce
- Start-up calibration for the touch pin and the quiet sound level
- Non-blocking program using `millis()`, so both sensors keep updating while switching pages

## Hardware Components

| Component | Model |
|---|---|
| Microcontroller | ESP32-WROOM-32 DevKit (30-pin) |
| Breakout board | Custom ESP32 breakout board (Assignment 1) |
| Sensor 1 | DHT11 temperature and humidity sensor |
| Sensor 2 | KY-037 sound sensor |
| Display | 0.96" SSD1306 OLED, 128 × 64, I2C |
| Touch input | Male pin of a female-to-male jumper wire |
| Others | Jumper wires, USB cable |

## Pin Mapping

| Component | Module pin | ESP32 pin |
|---|---|---|
| DHT11 | DATA | GPIO 4 |
| KY-037 | AO | GPIO 34 |
| KY-037 | DO | Not connected |
| OLED | SDA | GPIO 21 |
| OLED | SCL | GPIO 22 |
| Touch pin | Male pin | GPIO 27 (T7) |
| All modules | VCC / GND | 3.3 V / GND (via breakout board V and G rails) |

> All modules are powered at **3.3 V** because the ESP32 GPIO pins are not 5 V tolerant.

## Software Requirements

- **Arduino IDE**
- **Board package:** esp32 by Espressif Systems
- **Board:** ESP32 Dev Module

### Libraries (install from Library Manager)

| Library | Author |
|---|---|
| DHT sensor library | Adafruit |
| Adafruit Unified Sensor | Adafruit |
| Adafruit GFX Library | Adafruit |
| Adafruit SSD1306 | Adafruit |
| Wire | Built into the ESP32 core |

## How to Upload

1. Install the board package and libraries listed above.
2. Open `Smart_Environment_Monitor/Smart_Environment_Monitor.ino` in Arduino IDE.
3. Select **Tools → Board → ESP32 Dev Module** and choose the correct port.
4. Click **Upload**. If it gets stuck on "Connecting...", hold the **BOOT** button on the ESP32.
5. Open the **Serial Monitor** at **115200 baud** to see the readings.

## Configuration

These settings are at the top of the code and can be adjusted:

| Setting | Default | Purpose |
|---|---|---|
| `TOUCH_RATIO` | [0.7] | Touched when the reading drops below this ratio of the normal value. Higher = more sensitive |
| `SOUND_THRESHOLD` | [40] | How much louder than the quiet level counts as "Sound Detected". Lower = more sensitive |
| `SOUND_MAX` | [500] | How much louder than the quiet level counts as 100 % |

## How to Use

1. Power on the ESP32. A **"Calibrating..."** message appears for about 2 seconds. Keep quiet and do not touch the pin during this time.
2. **Page 1 – Environment** shows the temperature, humidity and status (Normal / Hot / Cold / Humid / Dry).
3. Touch the pin to switch to **Page 2 – Sound Monitor**, which shows the sound level, a level bar and the sound status.
4. Touch again to return to Page 1.

> The sound level is a relative value compared with the quiet level measured at start-up. It is **not** a decibel (dB) measurement.

## Author

[Your Name] – [Matric Number]
