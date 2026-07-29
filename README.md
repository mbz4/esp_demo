# esp_demo

# UT-Nutiplaat R2 — workshop firmware

Probe and demo firmware for the UT-Nutiplaat R2 development board, developed by students for students at the University of Tartu course Smart Solutions, led by prof [Heiki Kasemägi (etis CV link)](https://www.etis.ee/CV/Heiki_Kasem%C3%A4gi/eng/).

Exercises every on-board peripheral over a serial menu: the dual 7-segment display,
four addressable LEDs, rotary encoder, potentiometer, I²C bus, ultrasonic sensor and PIR.

---

## Quick start — no installation

The board ships flashed. You only need a browser and a USB-C cable.

1. Plug the board in over USB-C
2. Open the [serial terminal](https://rsc.ee/summer-school.html) (Course Detail → Open Serial Terminal)
3. Press **Connect** and pick the device
4. Press **Self test**

Works in Chrome, Edge, Opera and Firefox 151+. In Brave, first enable
`brave://flags/#brave-web-serial-api` and relaunch.

On Linux, if no device appears: `sudo usermod -aG dialout $USER`, then log out and back in.

**One browser tab per board.** Two tabs contending for one port is the most common failure.

---

## Serial commands

| Key | Action |
|---|---|
| `t` | Self test — display sweep and pixel chase |
| `k` | Knobs — potentiometer and encoder, 30 s |
| `i` | Scan the I²C bus |
| `u` | Ultrasonic distance, 15 s (header J6) |
| `r` | PIR motion, 15 s (header J2) |
| `n` | Network status, IP, RSSI, MAC |
| `W` | Set WiFi credentials |
| `F` | Forget WiFi credentials |
| `?` | Show the menu |

Credentials live in NVS, not in source. Press `W`, enter the SSID and password,
confirm with `y`. They survive reflashing — only `pio run -t erase` or `F` clears them.

---

## Building from source

Requires [PlatformIO](https://platformio.org/). The board needs Arduino core 3.x,
so this uses the [pioarduino](https://github.com/pioarduino/platform-espressif32)
platform rather than PlatformIO's official one.

```bash
git clone <this-repo>
cd esp_demo
pio run              # first build downloads ~1 GB of toolchain
pio run -t upload
pio device monitor
```

### platformio.ini essentials

```ini
[env:wroom32e]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip
board = esp32dev
framework = arduino
monitor_speed = 115200
monitor_rts = 0
monitor_dtr = 0
board_build.partitions = huge_app.csv
```

`monitor_rts = 0` and `monitor_dtr = 0` are **not optional** — see troubleshooting below.

`huge_app.csv` gives a 3 MB app slot and boots on any module of 4 MB or more.
For the full 16 MB, a second environment uses `partitions_16mb.csv`; run
`pio run -t erase` when switching a board between partition tables.

---

## Pin map

Derived from the R2 schematic. Nothing here matches a stock ESP32 devkit — check
against the schematic rather than assuming.

| GPIO | Function | | GPIO | Function |
|---|---|---|---|---|
| 2 | Encoder button (active high, ext. pull-down) | | 21 | mikroBUS INT |
| 4 | SPI MOSI | | 22 | mikroBUS RST |
| 5 | SPI MISO | | 23 | mikroBUS PWM |
| 12 | 74HCT595 data clock | | 25 | I²C SDA |
| 13 | PIR sensor | | 26 | I²C SCL |
| 14 | 74HCT595 latch clock | | 27 | 74HCT595 data |
| 15 | WS2812 data (via 74LV1T125) | | 32 | Ultrasonic trigger |
| 16 / 17 | mikroBUS UART RX / TX | | 33 | Ultrasonic echo |
| 18 / 19 | SPI SCK / CS | | 34 / 35 | Encoder B / A (input only) |
| 36 | mikroBUS analogue in | | 39 | Potentiometer (ADC1_CH3) |

**Reserved:** GPIO 6–11 carry the SPI flash. GPIO 1/3 carry UART0 to the USB bridge.

### On-board devices

- **DS3231M RTC** at I²C address `0x68`
- **Two 74HCT595** shift registers driving a dual common-cathode 7-segment display,
  16 outputs, no multiplexing
- **Four WS2812C** pixels, daisy-chained, 3.3 V → 5 V level shifted
- **Rotary encoder** with RC and Schottky debounce, pulled down so it can't disturb
  boot strapping
- **CP2102N** USB-UART bridge
- **AP63203** switching regulator, 5 V → 3.3 V

---

## Troubleshooting

### Serial silence, or the board freezes when a terminal connects

**On this board, RTS wires directly to EN and DTR to IO0** — through 10 Ω series
resistors, with no transistor pair like a standard devkit. Asserting RTS drives EN
low and holds the ESP32 in reset for as long as the port stays open.

Most tools assert both signals on open. Fixes:

- PlatformIO: `monitor_rts = 0` and `monitor_dtr = 0` in `platformio.ini`
- Web Serial: call `setSignals({dataTerminalReady: false, requestToSend: false})`
  immediately after `open()`
- The terminal in this repo does this already

### Boots to `entry 0x4008059c` and stops

The ROM handed off to the second-stage bootloader, then nothing. Usually a stale
partition table: `pio run -t upload` never erases what was there before, so leftover
otadata can point at a partition the new table doesn't define. Silent failure, since
the Arduino bootloader ships with logging suppressed.

```bash
pio run -t erase && pio run -t upload
```

### "Port is busy" or `Resource temporarily unavailable`

Something already holds the port — usually a monitor you forgot about.

```bash
pkill -f "platformio device monitor"
fuser -v /dev/ttyUSB0          # name the culprit
```

On Ubuntu, ModemManager also grabs new `ttyUSB` devices: `sudo systemctl stop ModemManager`.

### `[Errno 5] Input/output error`, then a reconnect loop

The USB device re-enumerated and got a new node — `ttyUSB0` became `ttyUSB1`.
PlatformIO binds once at startup and can't follow. Restart the monitor.

### `nvs_open failed: NOT_FOUND` on first boot

Expected. Opening the credentials namespace read-only before it exists. Stops once
you save credentials with `W`.

### WiFi won't connect

`n` shows the stored SSID in square brackets, which exposes trailing spaces.

- `201 NO_AP_FOUND` — SSID wrong, or out of range, or the AP is 5 GHz only
- `202 AUTH_FAIL` — password wrong

Press `W` to re-enter. The radio shuts down after a failed attempt so it can't spam
the console while you type.

### Typing doesn't appear in PlatformIO's monitor

The `time` filter buffers output until a newline. Drop it for interactive work:

```ini
monitor_filters = esp32_exception_decoder, log2file
```

---

## Licence

- **Code** — `src/`, `tools/`, the serial terminal: [MIT](LICENSE)
- **Teaching material** — slides, handouts, run sheets: [CC BY-SA 4.0](docs/LICENSE)

Third-party material keeps its own terms:

- KiCad handout by **evils** — [gitlab.com/evils/kicad-workshop](https://gitlab.com/evils/kicad-workshop),
  CERN-OHL-W v2
- UT-Nutiplaat R2 hardware design — University of Tartu, Institute of Technology

## Credits

Board designed at the University of Tartu Institute of Technology.
Firmware and workshop material for the 2026 summer school,
[Robot Study Companion](https://rsc.ee).

GPIO Viewer by [thelastoutpostworkshop](https://github.com/thelastoutpostworkshop/gpio_viewer).