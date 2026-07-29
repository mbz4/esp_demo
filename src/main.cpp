#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <gpio_viewer.h>

GPIOViewer gpio_viewer;

const char *WIFI_SSID = "RSC-2G";
const char *WIFI_PASS = "studybuddy";
constexpr uint32_t WIFI_TIMEOUT_MS = 15000;
constexpr int HEARTBEAT_PIN = 2;   // guess; census below will help confirm

// Output-capable and safe to drive on a WROOM-32E.
// Excluded: 6-11 (SPI flash), 1/3 (UART0), 0/2/12/15 (strapping), 34-39 (input only)
const uint8_t CANDIDATES[] = {4, 5, 13, 14, 16, 17, 18, 19,
                              21, 22, 23, 25, 26, 27, 32, 33};
constexpr size_t N_CAND = sizeof(CANDIDATES);

uint8_t pulledUp[N_CAND];
size_t nPulledUp = 0;

// External pull-ups (2.2-10k) beat the ESP32's internal ~45k, so a pin that
// still reads HIGH under an internal pull-down has something external on it.
void census() {
  Serial.println("\n--- pull census ---");
  nPulledUp = 0;
  for (size_t i = 0; i < N_CAND; i++) {
    uint8_t p = CANDIDATES[i];
    pinMode(p, INPUT_PULLDOWN);
    delayMicroseconds(500);
    bool extHigh = digitalRead(p);
    pinMode(p, INPUT_PULLUP);
    delayMicroseconds(500);
    bool extLow = !digitalRead(p);
    pinMode(p, INPUT);

    const char *verdict = extHigh ? "EXT PULL-UP" : (extLow ? "ext pull-down / driven low" : "floating");
    Serial.printf("  GPIO%-2u  %s\n", p, verdict);
    if (extHigh) pulledUp[nPulledUp++] = p;
  }
  Serial.printf("%u pin(s) carry an external pull-up\n", (unsigned)nPulledUp);
}

uint8_t probeBus(uint8_t sda, uint8_t scl, bool verbose) {
  Wire.end();
  if (!Wire.begin(sda, scl, 100000)) return 0;
  uint8_t found = 0;
  for (uint8_t addr = 8; addr < 120; addr++) {   // 0-7 and 120-127 are reserved
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      if (verbose) Serial.printf("    0x%02X\n", addr);
      found++;
    }
  }
  Wire.end();
  return found;
}

void scanI2C() {
  Serial.println("\n--- I2C: Arduino defaults 21/22 ---");
  uint8_t n = probeBus(21, 22, true);
  Serial.printf("  %u device(s)\n", n);

  if (nPulledUp < 2) {
    Serial.println("\n--- I2C sweep skipped: fewer than 2 pulled-up pins ---");
    return;
  }

  Serial.println("\n--- I2C sweep over pulled-up pins ---");
  for (size_t a = 0; a < nPulledUp; a++) {
    for (size_t b = 0; b < nPulledUp; b++) {
      if (a == b) continue;
      uint8_t sda = pulledUp[a], scl = pulledUp[b];
      uint8_t hits = probeBus(sda, scl, false);
      if (hits) {
        Serial.printf("  HIT  SDA=%u SCL=%u -> %u device(s):\n", sda, scl, hits);
        probeBus(sda, scl, true);
      }
      yield();
    }
  }
  Serial.println("sweep complete");
}

void setup() {
  Serial.begin(115200);
  delay(2000);                       // let the CP2102N settle before the banner
  pinMode(HEARTBEAT_PIN, OUTPUT);

  Serial.println("\n\n===== ALIVE =====");
  Serial.printf("chip rev %u, %u MHz, flash %u MB\n",
                ESP.getChipRevision(), getCpuFrequencyMhz(),
                ESP.getFlashChipSize() / (1024 * 1024));

  census();
  scanI2C();

  Serial.printf("\nconnecting to %s ", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
    delay(300);
    Serial.print('.');
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nIP %s\n", WiFi.localIP().toString().c_str());
    gpio_viewer.setSamplingInterval(50);
    gpio_viewer.setSkipPeripheralPins(false);
    gpio_viewer.begin();             // must stay last
  } else {
    Serial.printf("\nWiFi failed (status %d) - carrying on without the viewer\n",
                  WiFi.status());
  }

  Serial.println("\npress 's' to rescan");
}

void loop() {
  static uint32_t last = 0;
  static bool led = false;
  if (millis() - last > 500) {
    last = millis();
    digitalWrite(HEARTBEAT_PIN, led = !led);
  }
  if (Serial.available() && Serial.read() == 's') {
    census();
    scanI2C();
  }
}