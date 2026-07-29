# UT-Nutiplaat workshop — facilitator run sheet

**Format:** inverted classroom, 2 hours
**Group:** 8 students, 4 pairs, 5 boards (one spare)
**Board:** UT-Nutiplaat R2 — ESP32-WROOM-32E, 16 MB flash
**Terminal:** https://rsc.ee/summer-school.html → Course Detail → Open Serial Terminal

---

## Send tonight

Without this it isn't inverted, it's discovery learning — and you lose ten minutes to orientation.

> Tomorrow we take apart the board you'll build on. Before class, open the attached schematic (5 sheets) and answer three things:
>
> 1. How many separate chips can you find, and what does each one do?
> 2. Find one signal name that appears on more than one sheet. Which two things does it connect?
> 3. Pick one component you can't identify. Bring the question.
>
> Fifteen minutes, not an hour. Bring a laptop and a USB-C cable.

Attach: `UT-Nutiplaat_R2_Schematics.PDF`

---

## Prep checklist

- [ ] Flash all five boards with the probe firmware, set WiFi on each via `W`
- [ ] Label boards 1–5, record MAC from `n`
- [ ] Print the schematic, one copy per pair, sheets 1 and 2 at minimum
- [ ] Test the terminal on one Windows and one Mac laptop
- [x] Spare USB-C cables — charge-only cables are the classic session killer
- [ ] Print the pair-assignment sheets (from the slides)
- [ ] Confirm the room's wifi SSID matches what's stored on the boards

---

## Timetable

| Time | Block | Notes |
|---|---|---|
| 0:00 | Boards out, run the self test | The hook. Don't explain first — let them press `t` |
| 0:08 | PlatformIO walkthrough | 15 min hard stop, ends with homework |
| 0:23 | Activity 1 — subsystem jigsaw | Circulate, don't answer |
| 0:55 | Present back, 4 min each pair | Timer on screen |
| 1:15 | Break | |
| 1:20 | Activity 2 — sensor and bus | |
| 1:50 | Present back 2 min each + synthesis | Close with the four-bus slide |

**If you fall behind**, cut Activity 2's present-back to a single round-robin sentence per pair and go straight to the synthesis slide. Don't cut Activity 1 — it's the one that earns the rest.

---

## 0:00 — The hook (8 min)

Say nothing about architecture. Hand each pair a board and the terminal URL:

1. Plug in over USB-C
2. Open the terminal, press **Connect**, choose the device
3. Press **Self test**

Digits count, pixels chase. *Then* ask: "Three wires drive sixteen LEDs on that display. How?"

That question is Pair B's whole activity. Leave it hanging.

---

## 0:08 — PlatformIO walkthrough (15 min)

Frame it as "here's how I built the thing you just ran", not as a tutorial to follow along with. They watch; they install at home.

Show, in this order:

1. `platformio.ini` — one file names the board, the framework and the libraries
2. `pio run` — first build downloads a toolchain, later builds take seconds
3. `pio run -t upload` — the four images at 0x1000, 0x8000, 0xe000, 0x10000
4. `pio device monitor` — and why `monitor_rts = 0` matters on this board

**Homework, on the slide:** install VS Code and the pioarduino extension, clone the repo, run `pio run` once. The first build pulls roughly a gigabyte. Eight laptops doing that simultaneously on university wifi tomorrow costs you half an hour.

---

## 0:23 — Activity 1: subsystem jigsaw (32 min)

Two minutes of schematic-reading rules from the slide, then pairs work.

Each pair gets one subsystem and three tiered questions. Level 1 needs only eyes and the board, so nobody freezes. Level 2 is the real skill. Level 3 catches early finishers.

| Pair | Subsystem | Sheets |
|---|---|---|
| A | Getting code in — USB, UART bridge, reset | 1 |
| B | The number display — shift registers | 2 |
| C | The colourful bits — pixels and encoder | 2 |
| D | Plugging things in — headers, I²C, power | 3 + 4 |

**Your job while they work:** circulate and ask questions back. "Where does that wire go?" beats telling them. The one thing worth stating outright, because it isn't discoverable: nets connect by *name*, not by drawn line.

---

## 0:55 — Present back (20 min)

Four minutes each, timed and enforced. Three slides, fixed headings from the template:

1. **What it does** — plain language
2. **How it works** — the schematic evidence
3. **What surprised us**

Slide three matters most. It's where the actual learning surfaces.

---

## 1:20 — Activity 2: sensor and bus (30 min)

Each pair gets a physical sensor and answers the same five questions, then proves it on the board.

1. Which bus does it use?
2. Which header does it plug into?
3. How many wires, and what does each do?
4. Does it have an address? How do you find out?
5. What changes in the code?

The `[i]` scan is the payoff for anything on I²C — the device appears in a list, addressed and named.

**If you're short on distinct sensors:** give two pairs the same sensor on different headers and ask why it still works. That's arguably the better question anyway.

---

## 1:50 — Synthesis (10 min)

One round-robin sentence per pair, then the closing slide. The point lands on its own because this board carries all four models at once:

- **I²C** — two wires, many devices, each with an address
- **SPI** — four wires, one chip-select per device
- **UART** — two wires, exactly two participants, no addressing
- **No bus at all** — WS2812 and the ultrasonic sensor use raw timing

Same board, four answers to "how do two chips talk?"

---

## Live troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| Board freezes when the page connects | Someone ticked **RTS** — it wires to EN | Untick it |
| "Port is busy" | Two tabs, or PlatformIO still holding it | Close the other tab |
| Nothing in the device picker | Brave without the flag | `brave://flags/#brave-web-serial-api`, relaunch |
| Nothing in the picker, Linux laptop | Not in `dialout` group | `sudo usermod -aG dialout $USER`, log out and back in |
| Board resets randomly | Charge-only USB cable | Swap it |
| WiFi won't connect | Wrong SSID or password | Press `W`, retype, `y` |
| Display blank after a test | Test ended and blanked it | Press `t` |

**Say this out loud early:** one browser tab per board. Two tabs fighting over one port produced most of a morning's confusion during development.

---

## Tomorrow

PlatformIO on their own machines, then their own firmware. The `[i]` scan finding the RTC at `0x68` is the natural bridge into day two — read the time, show it on the display, then push it over MQTT.

**Check the MACs tonight.** If any two boards share one, MQTT client IDs derived from MAC will collide and silently kick each other off the broker.