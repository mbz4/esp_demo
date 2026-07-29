#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include "esp_cpu.h"

// ---------------- UT-Nutiplaat R2 pin map ----------------
constexpr uint8_t SR_DATA   = 27;   // 74HCT595 DI
constexpr uint8_t SR_CLK    = 12;   // 74HCT595 SCLK
constexpr uint8_t SR_LATCH  = 14;   // 74HCT595 LCLK
constexpr uint8_t WS2812    = 15;   // 4x WS2812C via 74LV1T125
constexpr uint8_t ENC_A     = 35;
constexpr uint8_t ENC_B     = 34;
constexpr uint8_t ENC_BTN   = 2;    // active HIGH, external pull-down
constexpr uint8_t POT       = 39;
constexpr uint8_t I2C_SDA   = 25;
constexpr uint8_t I2C_SCL   = 26;
constexpr uint8_t US_TRIG   = 32;
constexpr uint8_t US_ECHO   = 33;
constexpr uint8_t PIR       = 13;

const char *WIFI_SSID = "RSC-2G";
const char *WIFI_PASS = "studybuddy";

// ---------------- 7-segment ----------------
// seg bits: 0=a 1=b 2=c 3=d 4=e 5=f 6=g 7=dp   (common cathode, active high)
const uint8_t FONT[10] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};

// U9 outputs Q0..Q7 -> E D C DP B A G F
static uint8_t mapU9(uint8_t s) {
  uint8_t o = 0;
  if (s & 0x10) o |= 1 << 0;  if (s & 0x08) o |= 1 << 1;
  if (s & 0x04) o |= 1 << 2;  if (s & 0x80) o |= 1 << 3;
  if (s & 0x02) o |= 1 << 4;  if (s & 0x01) o |= 1 << 5;
  if (s & 0x40) o |= 1 << 6;  if (s & 0x20) o |= 1 << 7;
  return o;
}
// U10 outputs Q0..Q7 -> B A F E D G C DP
static uint8_t mapU10(uint8_t s) {
  uint8_t o = 0;
  if (s & 0x02) o |= 1 << 0;  if (s & 0x01) o |= 1 << 1;
  if (s & 0x20) o |= 1 << 2;  if (s & 0x10) o |= 1 << 3;
  if (s & 0x08) o |= 1 << 4;  if (s & 0x40) o |= 1 << 5;
  if (s & 0x04) o |= 1 << 6;  if (s & 0x80) o |= 1 << 7;
  return o;
}

void srInit() {
  pinMode(SR_DATA, OUTPUT); pinMode(SR_CLK, OUTPUT); pinMode(SR_LATCH, OUTPUT);
  digitalWrite(SR_CLK, LOW); digitalWrite(SR_LATCH, LOW);
}

// U9 is first in the chain, so U10's byte must be shifted out first.
void segRaw(uint8_t segA, uint8_t segB) {
  digitalWrite(SR_LATCH, LOW);
  shiftOut(SR_DATA, SR_CLK, MSBFIRST, mapU10(segB));
  shiftOut(SR_DATA, SR_CLK, MSBFIRST, mapU9(segA));
  digitalWrite(SR_LATCH, HIGH);
}

void segNumber(int v) {
  v = constrain(v, 0, 99);
  segRaw(FONT[v / 10], FONT[v % 10]);
}

void segBlank() { segRaw(0, 0); }

// ---------------- WS2812 (4 pixels, GRB) ----------------
static void wsShow(const uint8_t *b, size_t n) {
  const uint32_t T0H = 84, T1H = 168, TOTAL = 300;   // cycles @240 MHz
  const uint32_t mask = 1UL << WS2812;
  portDISABLE_INTERRUPTS();
  for (size_t i = 0; i < n; i++)
    for (int k = 7; k >= 0; k--) {
      uint32_t hi = ((b[i] >> k) & 1) ? T1H : T0H;
      uint32_t t0 = esp_cpu_get_cycle_count();
      REG_WRITE(GPIO_OUT_W1TS_REG, mask);
      while (esp_cpu_get_cycle_count() - t0 < hi) {}
      REG_WRITE(GPIO_OUT_W1TC_REG, mask);
      while (esp_cpu_get_cycle_count() - t0 < TOTAL) {}
    }
  portENABLE_INTERRUPTS();
  delayMicroseconds(300);
}

uint8_t px[12];
void pixelSet(int i, uint8_t r, uint8_t g, uint8_t b) {
  if (i < 0 || i > 3) return;
  px[i * 3] = g; px[i * 3 + 1] = r; px[i * 3 + 2] = b;
}
void pixelShow() { wsShow(px, sizeof(px)); }
void pixelClear() { memset(px, 0, sizeof(px)); pixelShow(); }

// ---------------- tests ----------------
void selfTest() {
  Serial.println(F("\n--- SELF TEST ---"));
  Serial.println(F("  7-seg: all segments"));
  segRaw(0xFF, 0xFF); delay(1200);
  Serial.println(F("  7-seg: 00-99"));
  for (int v = 0; v <= 99; v += 3) { segNumber(v); delay(40); }
  segNumber(42); delay(600);

  Serial.println(F("  WS2812: R G B W chase"));
  const uint8_t C[4][3] = {{80,0,0},{0,80,0},{0,0,80},{60,60,60}};
  for (int c = 0; c < 4; c++)
    for (int i = 0; i < 4; i++) {
      pixelClear(); pixelSet(i, C[c][0], C[c][1], C[c][2]); pixelShow(); delay(140);
    }
  pixelClear();
  segBlank();
  Serial.println(F("  done"));
}

void i2cScan() {
  Serial.println(F("\n--- I2C @ 25/26 ---"));
  Wire.end(); Wire.begin(I2C_SDA, I2C_SCL, 100000);
  uint8_t n = 0;
  for (uint8_t a = 8; a < 120; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  0x%02X%s\n", a, a == 0x68 ? "   <- DS3231M RTC" : "");
      n++;
    }
  }
  Serial.printf("  %u device(s)\n", n);
}

long usDistanceMm() {
  pinMode(US_TRIG, OUTPUT); pinMode(US_ECHO, INPUT);
  digitalWrite(US_TRIG, LOW);  delayMicroseconds(4);
  digitalWrite(US_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(US_TRIG, LOW);
  unsigned long us = pulseIn(US_ECHO, HIGH, 30000UL);
  return us ? (long)(us * 343L / 2000L) : -1;
}

void usTest() {
  Serial.println(F("\n--- ULTRASONIC 15 s (J6) ---"));
  uint32_t t0 = millis();
  while (millis() - t0 < 15000) {
    long mm = usDistanceMm();
    if (mm < 0) { Serial.println(F("  timeout (sensor fitted?)")); segBlank(); }
    else { Serial.printf("  %ld mm\n", mm); segNumber(mm / 10); }
    delay(300);
  }
  segBlank();
}

void pirTest() {
  Serial.println(F("\n--- PIR 15 s (J2) ---"));
  pinMode(PIR, INPUT);
  bool last = digitalRead(PIR);
  uint32_t t0 = millis();
  while (millis() - t0 < 15000) {
    bool now = digitalRead(PIR);
    if (now != last) { Serial.printf("  PIR -> %s\n", now ? "MOTION" : "clear"); last = now; }
    delay(20);
  }
}

void knobs() {
  Serial.println(F("\n--- KNOBS 30 s: pot -> display+pixels, encoder -> count ---"));
  pinMode(ENC_A, INPUT); pinMode(ENC_B, INPUT); pinMode(ENC_BTN, INPUT);
  static const int8_t QTAB[16] = {0,-1,1,0, 1,0,0,-1, -1,0,0,1, 0,1,-1,0};
  uint8_t prev = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
  int32_t pos = 0, shown = -999;
  bool btn = digitalRead(ENC_BTN), lastBtn = btn;
  bool potMode = true;
  uint32_t t0 = millis(), tick = 0;

  while (millis() - t0 < 30000) {
    uint8_t now = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
    if (now != prev) { pos += QTAB[(prev << 2) | now]; prev = now; }
    btn = digitalRead(ENC_BTN);
    if (btn && !lastBtn) { potMode = !potMode; Serial.printf("  button -> %s\n", potMode ? "pot" : "encoder"); }
    lastBtn = btn;

    if (millis() - tick > 100) {
      tick = millis();
      int val = potMode ? map(analogRead(POT), 0, 4095, 0, 99)
                        : (int)((pos / 2) % 100 + 100) % 100;
      if (val != shown) {
        shown = val;
        segNumber(val);
        int lit = val / 25;
        pixelClear();
        for (int i = 0; i <= lit && i < 4; i++) pixelSet(i, 0, 40, 20);
        pixelShow();
        Serial.printf("  %s = %2d   (enc raw %ld)\n", potMode ? "pot" : "enc", val, pos);
      }
    }
  }
  segBlank(); pixelClear();
}

void menu() {
  Serial.println(F("\n[t] self test  [k] knobs  [i] I2C scan  [u] ultrasonic  [r] PIR  [?] menu"));
}

void setup() {
  Serial.begin(115200);
  delay(1200);
  Serial.println(F("\n\n===== UT-Nutiplaat R2 ====="));

  srInit(); segBlank();
  pinMode(WS2812, OUTPUT); pixelClear();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  Serial.printf("connecting to %s ", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 12000) { delay(300); Serial.print('.'); }
  Serial.println(WiFi.status() == WL_CONNECTED ? "\nconnected" : "\nno WiFi - serial only");
  if (WiFi.status() == WL_CONNECTED) Serial.println(WiFi.localIP());

  selfTest();
  i2cScan();
  menu();
}

void loop() {
  if (!Serial.available()) { delay(20); return; }
  switch (Serial.read()) {
    case 't': selfTest(); break;
    case 'k': knobs();    break;
    case 'i': i2cScan();  break;
    case 'u': usTest();   break;
    case 'r': pirTest();  break;
    case '?': menu();     break;
    default: return;
  }
  menu();
}