#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <ESP32Servo.h>
#include "MAX30105.h"

// --- WI-FI SETTINGS ---
const char* ssid     = "IoT_RIP";       // Must be 2.4 GHz network!
const char* password = "rudra12345";

// ─────────────────────────────────────────────────────────────────
//  HOW TO FIND YOUR LAPTOP IP:
//    Windows → open CMD → type: ipconfig
//    Look for "IPv4 Address" under your Wi-Fi adapter
//    Example: 10.101.142.194
//
//  YOUR BROWSER uses:  http://127.0.0.1:8000   (same laptop only)
//  ESP32 MUST use:     http://<your-laptop-ip>:8000
//                       ↑ 127.0.0.1 will NOT work here
// ─────────────────────────────────────────────────────────────────
const char* laptopApiUrl = "http://10.101.142.194:8000/api/update_sensor";

// --- HARDWARE PINS ---
#define TRIG_PIN      13
#define ECHO_PIN      25
#define SERVO_PIN     14
#define BUZZER_PIN    26
#define ENTRY_IR_PIN  32
#define EXIT_IR_PIN   27
#define RED_LED_PIN   16
#define GREEN_LED_PIN 17

WebServer espServer(80);
MAX30105  particleSensor;
Servo     vaultDoor;

// --- STATE VARIABLES ---
unsigned long lastSendTime  = 0;
long          lastDist      = -1;
unsigned long buzzerOffTime = 0;

int  entryCount   = 0;
int  exitCount    = 0;
int  peopleInside = 0;
int  currentAlert = 0;

bool entryState = false;
bool exitState  = false;

// ─────────────────────────────────────────────────
// Robust WiFi connect with timeout + retry
// ─────────────────────────────────────────────────
void connectWiFi() {
  Serial.println("\n[WiFi] Connecting to: " + String(ssid));

  WiFi.disconnect(true);   // clear any previous config
  delay(100);
  WiFi.mode(WIFI_STA);     // station mode (not AP)
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;

    if (attempts >= 30) {   // 15 seconds timeout
      Serial.println("\n[WiFi] FAILED to connect. Check:");
      Serial.println("  1. Is the network 2.4 GHz? (ESP32 does not support 5 GHz)");
      Serial.println("  2. Is the password correct?");
      Serial.println("  3. Is the router in range?");
      Serial.println("[WiFi] Retrying in 5 seconds...");
      delay(5000);
      attempts = 0;
      WiFi.begin(ssid, password);
    }
  }

  Serial.println("\n[WiFi] Connected!");
  Serial.print("[WiFi] ESP32 IP Address : "); Serial.println(WiFi.localIP());
  Serial.print("[WiFi] Signal strength  : "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
  Serial.println("[WiFi] Sending data to : " + String(laptopApiUrl));
  Serial.println("[WiFi] Open website at : http://127.0.0.1:8000  (on your laptop browser)");
}

// ─────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("--- NEXUS VAULT STARTING ---");

  pinMode(TRIG_PIN,      OUTPUT);
  pinMode(ECHO_PIN,      INPUT);
  pinMode(BUZZER_PIN,    OUTPUT);
  pinMode(ENTRY_IR_PIN,  INPUT);
  pinMode(EXIT_IR_PIN,   INPUT);
  pinMode(RED_LED_PIN,   OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN,    LOW);
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN,   LOW);

  vaultDoor.attach(SERVO_PIN);
  vaultDoor.write(0);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("WARNING: MAX30105 not found.");
  } else {
    particleSensor.setup();
  }

  connectWiFi();   // connect with retry loop

  espServer.on("/open_door", []() {
    espServer.send(200, "text/plain", "OK");
    Serial.println("Authorized! Opening Door.");
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN,   HIGH);
    vaultDoor.write(90);
    delay(6000);
    vaultDoor.write(0);
    digitalWrite(RED_LED_PIN,   LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
  });

  espServer.begin();
}

// ─────────────────────────────────────────────────
long readDistance() {
  digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH, 30000);
  if (dur == 0) return -1;
  return dur * 0.034 / 2;
}

// ─────────────────────────────────────────────────
void loop() {
  // Auto-reconnect if WiFi drops
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Connection lost! Reconnecting...");
    connectWiFi();
  }

  espServer.handleClient();

  long distance = readDistance();

  // --- Entry IR ---
  bool currentEntry = (digitalRead(ENTRY_IR_PIN) == LOW);
  if (currentEntry && !entryState) {
    entryCount++;
    peopleInside++;
    entryState = true;
    Serial.println("Person Entered. Inside: " + String(peopleInside));
  } else if (!currentEntry) {
    entryState = false;
  }

  // --- Exit IR ---
  bool currentExit = (digitalRead(EXIT_IR_PIN) == LOW);
  if (currentExit && !exitState) {
    if (lastDist > 0 && distance > 0 && distance < lastDist && distance < 100) {
      Serial.println("ALERT: Wrong way at exit!");
      digitalWrite(BUZZER_PIN, HIGH);
      buzzerOffTime = millis() + 500;
      currentAlert  = 1;
    } else {
      if (peopleInside > 0) {
        exitCount++;
        peopleInside--;
        Serial.println("Person Exited. Inside: " + String(peopleInside));
      }
    }
    exitState = true;
  } else if (!currentExit) {
    exitState = false;
  }

  if (distance > 0) lastDist = distance;

  // --- Biometric sensor ---
  long irValue = particleSensor.getIR();
  if (irValue > 50000 && peopleInside == 0) {
    Serial.println("ALERT: Unauthorized touch!");
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerOffTime = millis() + 500;
    currentAlert  = 2;
    irValue       = 0;
  }

  if (millis() > buzzerOffTime && millis() > 100) {
    digitalWrite(BUZZER_PIN, LOW);
    currentAlert = 0;
  }

  // --- POST sensor data every 500 ms ---
  if (millis() - lastSendTime > 500) {
    lastSendTime = millis();

    HTTPClient http;
    http.begin(laptopApiUrl);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(2000);   // 2 s timeout so slow replies don't stall the loop

    long distToSend = (distance > 0) ? distance : 0;

    String json = "{\"distance\":"    + String(distToSend)  +
                  ",\"ir_value\":"    + String(irValue)      +
                  ",\"entry_count\":" + String(entryCount)   +
                  ",\"exit_count\":"  + String(exitCount)    +
                  ",\"people_inside\":" + String(peopleInside) +
                  ",\"alert_type\":" + String(currentAlert)  + "}";

    int httpCode = http.POST(json);
    if (httpCode == 200) {
      Serial.println("[POST] OK → data sent");
    } else {
      Serial.printf("[POST] Failed: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  }
}
