#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <ESP32Servo.h>
#include "MAX30105.h"

// --- WI-FI SETTINGS ---
const char* ssid = "IoT_RIP";
const char* password = "rudra12345";

// PUT YOUR LAPTOP'S IP ADDRESS HERE
const char* laptopApiUrl = "http://10.101.142.16:8000/api/update_sensor"; 

// --- HARDWARE PINS ---
#define TRIG_PIN 13
#define ECHO_PIN 25
#define SERVO_PIN 14
#define BUZZER_PIN 26
#define ENTRY_IR_PIN 32    // IR sensor for Entry
#define EXIT_IR_PIN 27     // IR sensor for Exit

// FIXED PINS: Avoided 26 (Conflict) and 34 (Input-only)
#define RED_LED_PIN 16     // Vault Open LED
#define GREEN_LED_PIN 17   // Vault Closed LED

WebServer espServer(80);
MAX30105 particleSensor;
Servo vaultDoor;

// --- STATE VARIABLES ---
unsigned long lastSendTime = 0;
long lastDist = -1;              
unsigned long buzzerOffTime = 0; 

int entryCount = 0;
int exitCount = 0;
int peopleInside = 0;

bool entryState = false;
bool exitState = false;

void setup() {
  Serial.begin(115200);
  Serial.println("--- NEXUS VAULT STARTING ---");
  
  // Pin Modes
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ENTRY_IR_PIN, INPUT);
  pinMode(EXIT_IR_PIN, INPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  
  // Buzzer Default Off
  digitalWrite(BUZZER_PIN, LOW);
  
  // Default Vault State (Closed = Green LED ON, Red LED OFF)
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN, LOW);
  vaultDoor.attach(SERVO_PIN);
  vaultDoor.write(0); 

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("ERROR: MAX30105 Biometric Sensor missing.");
  } else {
    particleSensor.setup(); 
  }

  // Connect Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Connected!");
  Serial.print("ESP32 IP Address: "); Serial.println(WiFi.localIP());

  // Handle Door Open Command from Python Server
  espServer.on("/open_door", []() {
    espServer.send(200, "text/plain", "OK");
    Serial.println("Authorized! Opening Door.");
    
    // Vault is OPEN -> Red ON, Green OFF
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, HIGH);
    vaultDoor.write(90); 
    
    delay(6000); // Keep open for 6 seconds
    
    // Vault is CLOSED -> Green ON, Red OFF
    vaultDoor.write(0); 
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
  });

  espServer.begin();
}

void loop() {
  // Listen for Python API requests to open the door
  espServer.handleClient();

  // 1. Read Ultrasonic Distance (Exit Sensor)
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  long distance = duration * 0.034 / 2;

  // 2. Track Entry / Exit using IR Sensors
  bool currentEntry = digitalRead(ENTRY_IR_PIN) == LOW; 
  if (currentEntry && !entryState) {
    entryCount++;
    peopleInside++;
    entryState = true;
    Serial.println("Person Entered.");
  } else if (!currentEntry) {
    entryState = false;
  }

  bool currentExit = digitalRead(EXIT_IR_PIN) == LOW;
  if (currentExit && !exitState) {
    // EDGE CASE 1: Person triggered exit IR, but distance is decreasing (Walking IN through EXIT)
    if (lastDist > 0 && distance > 0 && distance < lastDist && distance < 100) {
      Serial.println("ALERT: Wrong way detected at exit!");
      digitalWrite(BUZZER_PIN, HIGH); // Buzzer ON
      buzzerOffTime = millis() + 2000; 
    } else {
      if (peopleInside > 0) {
        exitCount++;
        peopleInside--;
        Serial.println("Person Exited.");
      }
    }
    exitState = true;
  } else if (!currentExit) {
    exitState = false;
  }

  // 3. Heart Rate Biometric Logic
  long irValue = particleSensor.getIR();
  
  if (irValue > 50000) {
    // EDGE CASE 2: No one inside, but sensor touched
    if (peopleInside == 0) {
      Serial.println("ALERT: Unauthorized biometric touch! Vault is empty.");
      digitalWrite(BUZZER_PIN, HIGH); // Buzzer ON
      buzzerOffTime = millis() + 2000; 
      
      irValue = 0; // Mask the value so Python server doesn't show login popup
    }
  }

  // 4. Handle Non-Blocking Buzzer Turn-Off
  if (millis() > buzzerOffTime) {
    digitalWrite(BUZZER_PIN, LOW); // Buzzer OFF
  }
  
  lastDist = distance;

  // 5. Send Data to Python Server Every 500ms
  if (millis() - lastSendTime > 500) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(laptopApiUrl);
      http.addHeader("Content-Type", "application/json");
      
      String json = "{\"distance\":" + String(distance) + 
                    ",\"ir_value\":" + String(irValue) + 
                    ",\"entry_count\":" + String(entryCount) + 
                    ",\"exit_count\":" + String(exitCount) + 
                    ",\"people_inside\":" + String(peopleInside) + "}";
                    
      http.POST(json);
      http.end();
    }
    lastSendTime = millis();
  }
}