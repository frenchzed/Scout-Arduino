# Scout32 firmware (ESP32 Arduino core 3.x port)

This is the firmware for the **[Scout32](https://www.pilothobbies.com/product/scout32/)**, a small
3D-printed tracked FPV rover designed by **Pilot Hobbies** (Pilotgeek). It runs on an
**ESP32-CAM** and uses an **MX1508** dual H-bridge to drive two N20 gear motors. When it boots, the rover
creates its own Wi-Fi network and serves a web page with a live camera feed and driving controls.
You drive it from a phone or computer browser.

This repo is the original Pilot Hobbies `Scout32MX1508` sketch (v2.3), **ported to ESP32 Arduino
core 3.x**. The upstream code only compiles against the old ESP32 core 1.0.6. This port builds
with current toolchains (tested with core **3.3.12**).

## Credits & upstream

All credit for the Scout32 design, the 3D models, the electronics layout and the original firmware
goes to **Pilot Hobbies**:

- Product page, STL files, manual and build video: **https://www.pilothobbies.com/product/scout32/**
- YouTube: https://www.youtube.com/user/Pilotgeek
- Contact: see the product page

The upstream archives are imported verbatim in this repo's git history and tagged, so you can diff
against them (for example `git diff v2.3`):

| Tag    | Source |
|--------|--------|
| `v2.2` | [`Scout32-ScoutXL_2.2.zip`](https://www.pilothobbies.com/wp-content/uploads/2022/05/Scout32-ScoutXL_2.2.zip) |
| `v2.3` | [`Scout-Arduino-2.3.zip`](https://www.pilothobbies.com/wp-content/uploads/2024/05/Scout-Arduino-2.3.zip) |

Get the **STL files, the build manual and the wiring diagrams** from the Pilot Hobbies product
page. This repo contains only the firmware.

### License

**The upstream license is unclear.** The Pilot Hobbies product page and the downloaded archives
include no license, copyright notice or terms of use. Until the author states otherwise, treat the
upstream code as **all rights reserved by Pilot Hobbies**. This repo shares it in good faith, for
personal and hobby use only. If you are the author and want this changed or taken down, please open
an issue.

Parts of `app_server.cpp` (the HTTP server, the MJPEG streaming and the capture handlers) come from
Espressif's `CameraWebServer` example in
[arduino-esp32](https://github.com/espressif/arduino-esp32), which is licensed under Apache-2.0.

## What changed in the 3.x port

- Uses the new LEDC API (`ledcAttachChannel()`, and `ledcWrite()` by pin instead of by channel).
- Motor PWM runs at 1000 Hz. The original asked for 8000 Hz at 16-bit resolution, which the hardware
  cannot do; core 1.0.6 silently ran it at about 1220 Hz instead.
- Explicit PWM channels 2, 3 and 4, so the motors and LED never collide with the camera XCLK on
  channel/timer 0.
- Camera config is zero-initialised and uses `grab_mode` and `fb_location` (`CAMERA_GRAB_LATEST`
  for lower latency when PSRAM is present). Uses the renamed `pin_sccb_*` fields.
- Removes the `dl_lib` dependency, which no longer exists in core 3.x. `/capture` now returns the
  sensor JPEG directly.

Driving behaviour, the web UI and the pinout are unchanged from upstream v2.3.

## Hardware

| Part | Notes |
|------|-------|
| ESP32-CAM (AI-Thinker) with OV2640 | Default target. WROVER boards need pin changes, see below |
| MX1508 dual DC motor driver | Recommended by upstream over the older servo/ESC variant |
| 2× N20 gear motors | Tracks |
| USB-to-serial adapter, or an ESP32-CAM-MB "programmer" board | For flashing. The ESP32-CAM has no USB port |
| Power | For example a 1S LiPo with a 5 V booster. See the Pilot Hobbies manual |

### Wiring (ESP32-CAM → MX1508)

| ESP32-CAM GPIO | MX1508 | Function |
|--------|--------|----------|
| GPIO12 | IN1 | Left motor forward (PWM) |
| GPIO13 | IN2 | Left motor reverse |
| GPIO15 | IN3 | Right motor forward (PWM) |
| GPIO14 | IN4 | Right motor reverse |
| GPIO4  | (on-board) | Flash LED, used as headlights |

Power the ESP32-CAM and the MX1508 from 5 V with a **common ground**. If a track spins the wrong
way, swap that motor's two wires on the MX1508 output.

To change the pins, edit the `Motor*` and `LEDPIN` constants near the top of
`sketchbook/Scout32MX1508/Scout32MX1508.ino`.

## Configure

Before building, open `sketchbook/Scout32MX1508/Scout32MX1508.ino` and check the settings at the top:

```cpp
bool ap = 1;                       // 1 = rover is its own access point (192.168.4.1)
                                   // 0 = join an existing Wi-Fi network (DHCP)
const char* ssid = "Scout32";      // AP name, or your router's SSID when ap = 0
const char* hostname = "Scout32";
const char* password = "";         // empty = open network (AP mode)
int channel = 11;                  // Wi-Fi channel in AP mode
int maxconnection = 1;             // one driver at a time
```

**Set a password** if you don't want anyone nearby to be able to take control of your rover. In AP
mode the password must be empty or at least 8 characters long.

## Build & flash

You need the **esp32 by Espressif Systems** board package, **version 3.x**. Version 1.0.6/2.x will
not compile this. Select the **AI Thinker ESP32-CAM** board.

### Put the ESP32-CAM in flash mode

- **ESP32-CAM-MB adapter:** plug it in. Most boards enter flash mode automatically. If yours
  doesn't, hold **IO0**, press **RST**, then release IO0.
- **FTDI / USB-serial adapter (3.3 V logic):** wire adapter TX→U0R, RX→U0T, GND→GND, 5 V→5 V.
  **Bridge GPIO0 to GND**, then press reset. Once flashing has finished, remove the bridge and press
  reset again to run the firmware.

### Option A: Arduino IDE

1. *File → Preferences → Additional boards manager URLs*: add
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. *Tools → Board → Boards Manager*: install **esp32** by Espressif, **3.x**.
3. Open `sketchbook/Scout32MX1508/Scout32MX1508.ino`.
4. *Tools → Board*: **AI Thinker ESP32-CAM**. *Tools → Port*: your adapter.
5. Click **Upload**.

### Option B: arduino-cli

```sh
arduino-cli config add board_manager.additional_urls \
  https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32

# build
arduino-cli compile --fqbn esp32:esp32:esp32cam sketchbook/Scout32MX1508

# flash (adjust the port: /dev/ttyUSB0, /dev/ttyACM0, COM3, ...)
arduino-cli upload --fqbn esp32:esp32:esp32cam -p /dev/ttyUSB0 sketchbook/Scout32MX1508

# watch the boot log
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

On Linux you may need to be in the `dialout` (or `uucp`) group to access the serial port.

## Drive it

1. Power the rover. When it is ready, the headlight LED **blinks 5 times**.
2. Connect your phone or laptop to the Wi-Fi network **`Scout32`**.
3. Open **http://192.168.4.1** in a browser. In client mode (`ap = 0`), use the IP address printed
   on the serial monitor, or `http://Scout32` if your network resolves hostnames.
4. Drive with the **arrow keys**, or hold the on-screen **Forward / Back / Left / Right** buttons.

The page also has these controls:

| Control | What it does |
|---------|--------------|
| Speed | Motor speed, from 0 to 8 |
| Trim | Corrects drift if one track is faster than the other |
| Lights | Headlight (flash LED) brightness |
| Quality / Resolution | JPEG quality and frame size of the video |
| Rotate Cam | Rotates the video by 90° to match how the camera is mounted |

**Failsafe:** if the rover receives no command for **4 seconds**, for example because the browser
closed or Wi-Fi dropped, it stops the motors.

The video stream is also available on its own at `http://192.168.4.1:81/stream`, and a single
still frame at `http://192.168.4.1/capture`.

## ESP32-WROVER boards

The sketch includes an alternative camera pin block for ESP32-WROVER boards. To use it:

- comment out the *ESP32-CAM (Standard)* camera pin block and uncomment the *ESP32-WROVER* block;
- move the LED to GPIO2 (`LEDPIN = 2`), otherwise the board crashes;
- search the sketch for `WROVER` to find the other notes.

## Troubleshooting

- **`Camera init failed with error 0x...`**: check the camera ribbon cable and the board selection.
  An unstable power supply can also cause this.
- **Resets or brownouts when the motors start**: the motors pull the supply voltage down. Use a
  stronger 5 V source or add a capacitor across the supply. (The firmware already disables the
  brownout detector.)
- **Upload fails with `Failed to connect` or `Wrong boot mode`**: the board is not in flash mode.
  See [Put the ESP32-CAM in flash mode](#put-the-esp32-cam-in-flash-mode).
- **Compile errors about `ledcSetup`, `dl_lib` or `pin_sscb_sda`**: you are building the upstream
  code or using an old core. Use this repo with esp32 core 3.x.
- **Video lag or choppy stream**: lower Resolution or Quality on the web page, and keep the rover
  close. Only one client can connect at a time.
