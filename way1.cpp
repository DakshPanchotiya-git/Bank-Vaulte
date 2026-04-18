#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <ESP32Servo.h>
#include "MAX30105.h"
#include "heartRate.h"

// --- Wi-Fi Credentials ---
const char* ssid = "Bank_Vault_Secure";
const char* password = "AdminPassword";
WebServer server(80);

// --- Hardware Pins ---
#define IR_ENTRY_PIN 32
#define IR_EXIT_PIN 33
#define TRIG_PIN 18
#define ECHO_PIN 19
#define LDR_PIN 34
#define SERVO_PIN 14

// --- Global Variables & Objects ---
MAX30105 particleSensor;
Servo vaultDoor;

int entryCount = 0;
int exitCount = 0;
bool entryState = false;
bool exitState = false;

long lastBeat = 0;
float beatsPerMinute = 0;
int beatAvg = 0;
byte rates[4]; // Array of heart rates
byte rateSpot = 0;

bool doorOpen = false;
bool safeBreached = false;
int distanceCm = 0;

// The HTML Dashboard (Served if accessed via Web Browser)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Vault Security Interface</title>
  <style>
    body { background-color: #050914; color: #e2e8f0; font-family: 'Courier New', monospace; padding: 20px; text-align: center; }
    h1 { color: #fff; letter-spacing: 2px; }
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin-top: 20px; }
    .card { background: #0d1527; border: 1px solid #1e293b; padding: 15px; border-radius: 5px; box-shadow: inset 0 0 10px rgba(0,0,0,0.5); }
    .title { color: #64748b; font-size: 12px; margin-bottom: 5px; }
    .val { font-size: 24px; font-weight: bold; color: #00ff88; }
    .alert { color: #ff3333; }
  </style>
</head>
<body>
  <h1>VAULT DASHBOARD</h1>
  <div class="grid">
    <div class="card"><div class="title">BIOMETRIC</div><div class="val" id="bpm">-- BPM</div></div>
    <div class="card"><div class="title">VAULT DOOR</div><div class="val" id="door">LOCKED</div></div>
    <div class="card"><div class="title">OCCUPANCY</div><div class="val" id="occ">0</div></div>
    <div class="card"><div class="title">SAFE (LDR)</div><div class="val" id="ldr">SECURE</div></div>
  </div>
  <script>
    setInterval(async () => {
      let res = await fetch('/data');
      let data = await res.json();
      document.getElementById('bpm').innerText = data.bpm + " BPM";
      document.getElementById('occ').innerText = data.occupancy;
      
      let doorEl = document.getElementById('door');
      doorEl.innerText = data.doorOpen ? "UNLOCKED" : "LOCKED";
      doorEl.className = data.doorOpen ? "val alert" : "val";

      let ldrEl = document.getElementById('ldr');
      ldrEl.innerText = data.safeBreached ? "BREACH" : "SECURE";
      ldrEl.className = data.safeBreached ? "val alert" : "val";
    }, 1000);
  </script>
</body>
</html>
)rawliteral";

// --- API Endpoint ---
void handleData() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  String json = "{";
  json += "\"bpm\": " + String(beatAvg) + ",";
  json += "\"occupancy\": " + String(entryCount - exitCount) + ",";
  json += "\"distance\": " + String(distanceCm) + ",";
  json += "\"doorOpen\": " + String(doorOpen ? "true" : "false") + ",";
  json += "\"safeBreached\": " + String(safeBreached ? "true" : "false");
  json += "}";
  
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  
  // Initialize Pins
  pinMode(IR_ENTRY_PIN, INPUT);
  pinMode(IR_EXIT_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  
  vaultDoor.attach(SERVO_PIN);
  vaultDoor.write(0); // Lock door initially

  // Initialize MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 was not found. Please check wiring/power.");
  }
  particleSensor.setup(); 
  particleSensor.setPulseAmplitudeRed(0x0A);

  // Start Wi-Fi AP
  WiFi.softAP(ssid, password);
  Serial.print("System Active at IP: ");
  Serial.println(WiFi.softAPIP());

  // Server Routing
  server.on("/", []() { server.send(200, "text/html", index_html); });
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient(); // Keep web server responsive

  // 1. Read IR Sensors (Occupancy Tracking)
  bool currentEntry = digitalRead(IR_ENTRY_PIN) == LOW; // Assuming active LOW
  if (currentEntry && !entryState) { entryCount++; entryState = true; }
  else if (!currentEntry) { entryState = false; }

  bool currentExit = digitalRead(IR_EXIT_PIN) == LOW;
  if (currentExit && !exitState) { exitCount++; exitState = true; }
  else if (!currentExit) { exitState = false; }
  
  int occupancy = entryCount - exitCount;
  if (occupancy < 0) { entryCount = 0; exitCount = 0; } // Prevent negative numbers

  // 2. Read Pulse Sensor
  long irValue = particleSensor.getIR();
  if (checkForBeat(irValue) == true) {
    long delta = millis() - lastBeat;
    lastBeat = millis();
    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= 4;
      beatAvg = 0;
      for (byte x = 0 ; x < 4 ; x++) beatAvg += rates[x];
      beatAvg /= 4;
    }
  }
  if (irValue < 50000) { beatAvg = 0; } // Finger removed

  // 3. Vault Door Logic (Entry > Exit AND valid pulse)
  bool isAlive = (beatAvg > 50 && beatAvg < 130);
  if (occupancy > 0 && isAlive) {
    vaultDoor.write(90); // Open Door
    doorOpen = true;
  } else {
    vaultDoor.write(0);  // Close Door
    doorOpen = false;
  }

  // 4. LDR Safe Breach Logic
  int ldrVal = analogRead(LDR_PIN);
  // If light hits the LDR inside the dark safe, flag breach 
  safeBreached = (ldrVal > 2000); 

  delay(20); // Small delay for stability
}