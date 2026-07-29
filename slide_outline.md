# Workshop slides — UT-Nutiplaat

Each `---` marks a new slide. Speaker notes sit in blockquotes; don't put them on the slide.

---

## Take apart the board you'll build on

UT-Nutiplaat R2
Designing Social Robots for Human Interaction
Summer School 2026

> Hand out boards and cables while this is up.

---

## Today

**First hour** — pull the board apart. Four teams, four subsystems, one schematic.

**Second hour** — plug a sensor in and work out how it talks.

No installs. Everything runs in your browser.

---

## Plug in and press one button

1. Connect the board over **USB-C**
2. Open **rsc.ee/summer-school.html** → Course Detail → **Open Serial Terminal**
3. Press **Connect**, pick the device from the list
4. Press **Self test**

> Let them do this before any explanation. Wait for the pixels.

---

## Before you go further

Works in **Chrome, Edge, Opera, Firefox 151+**

**Brave** — open `brave://flags/#brave-web-serial-api`, set Enabled, relaunch

**Linux** — if no device appears: `sudo usermod -aG dialout $USER`, then log out and back in

**One browser tab per board.** Two tabs fighting over one port is the most common failure today.

---

## So how does that work?

Three wires from the processor.

Sixteen LEDs on the display.

> Leave the question hanging. It's Pair B's activity.

---

## Reading a schematic: four rules

**Letters name parts.** U = chip, R = resistor, J = connector, LED = display. `U9` is one specific chip.

**Names are wires.** Two labels with the same text are the same wire — even on different sheets, even with no line drawn between them.

**Yellow arrows cross sheets.** Follow the name, not the line.

**Sheet 1 is the brain.** Everything else hangs off it.

> Demonstrate live on Sheet 2. Trace `Data.CLK` from the ESP32 to U9.

---

## Your job

Follow one signal name from the chip to the thing it controls.

---

## Activity 1 — four teams, four subsystems

| Team | Subsystem | Sheets |
|---|---|---|
| **A** | Getting code in | 1 |
| **B** | The number display | 2 |
| **C** | The colourful bits | 2 |
| **D** | Plugging things in | 3 + 4 |

**30 minutes.** Then 4 minutes each to present.

Answer level 1 first. Everyone can. Level 3 only if you finish early.

---

## Team A — Getting code in

**1.** Find the USB connector and the two buttons on your board. What does each button do when you press it?

**2.** Two chips sit between the USB socket and the processor. Name them both. Which one is the radio?

**3.** *Stretch:* S1 and S2 connect to EN and IO0. The USB chip also connects to those two pins. What does that let your laptop do without you touching a button?

---

## Team B — The number display

**1.** How many segments light up on a single digit? Count them, including the dot.

**2.** Two identical chips drive the display. Find their part number. How many wires run from the processor to them — and how many LEDs do those wires control?

**3.** *Stretch:* Every LED has a 270 Ω resistor in series. What happens without it?

---

## Team C — The colourful bits

**1.** How many colour LEDs are on the board? Press `t` and watch them.

**2.** Find those LEDs on the schematic. How are they wired to each other — and how many processor pins does the whole chain use?

**3.** *Stretch:* The encoder knob has three capacitors and three diodes on it. Turn it fast while running `k`. What problem are those parts solving?

---

## Team D — Plugging things in

**1.** Count the headers on your board. Find the one labelled SDA / SCL.

**2.** Press `i`. Something answers at address `0x68`. Find it on Sheet 1 and say what it does. Which other headers share those same two wires?

**3.** *Stretch:* Four sockets, two wires. How do four devices avoid talking over each other?

---

## Terminal commands

| Key | Does |
|---|---|
| `t` | Self test — display and pixels |
| `k` | Knobs — pot and encoder, 30 s |
| `i` | Scan the I²C bus |
| `u` | Ultrasonic distance, 15 s |
| `r` | PIR motion, 15 s |
| `n` | Network status and MAC |
| `?` | Show the menu |

---

## Present back — 3 slides, 4 minutes

**1. What it does** — in plain language, no part numbers

**2. How it works** — show us the schematic evidence

**3. What surprised us** — the bit you didn't expect

> Slide 3 is the one that matters. Say so.

---

## How I built the thing you just ran

**platformio.ini** — one file names the board, the framework, the libraries

**pio run** — compile

**pio run -t upload** — four images, four addresses

**pio device monitor** — and why this board needs `monitor_rts = 0`

> Watch, don't follow along. You'll install it at home.

---

## Homework

1. Install **VS Code** and the **pioarduino** extension
2. Clone the repo
3. Run `pio run` once — let it finish

The first build downloads about a gigabyte of compiler.

**Do it on your own wifi tonight, not here tomorrow.**

---

## Activity 2 — one sensor each

Answer these five, then prove it on the board:

1. Which bus does it use?
2. Which header does it plug into?
3. How many wires, and what does each one do?
4. Does it have an address? How would you find out?
5. What changes in the code?

**30 minutes.** Then 2 minutes each.

---

## Four ways to talk — all on one board

**I²C** — two wires, many devices, every one has an address
*RTC, OLED, motion sensor*

**SPI** — four wires, plus one chip-select per device
*mikroBUS sockets*

**UART** — two wires, exactly two participants, no addresses
*your laptop to the board*

**No bus at all** — just precisely timed pulses
*the colour LEDs, the ultrasonic sensor*

> Ask which one they'd pick for twenty sensors on one cable, and why.

---

## What you found

| Pin | What's on it | | Pin | What's on it |
|---|---|---|---|---|
| 2 | Encoder button | | 25 | I²C SDA |
| 12 | Display data clock | | 26 | I²C SCL |
| 13 | PIR sensor | | 27 | Display data |
| 14 | Display latch | | 32 | Ultrasonic trigger |
| 15 | Colour LEDs | | 33 | Ultrasonic echo |
| 18/19 | SPI clock, select | | 34/35 | Encoder A, B |
| 4/5 | SPI data | | 39 | Potentiometer |

> Hold this back until after both present-backs. It's the answer key.

---

## Tomorrow

Your own toolchain. Your own firmware.

Read the clock chip. Show the time on the display.

Then send it somewhere.