#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <ESP32Servo.h>
#include "MAX30105.h"

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// PUT YOUR LAPTOP'S IP ADDRESS HERE
const char* laptopApiUrl = "http://192.168.x.x:8000/api/update_sensor"; 

#define TRIG_PIN 13
#define ECHO_PIN 25
#define SERVO_PIN 14
#define BUZZER_PIN 32 // Buzzer pin for exit detection alert

WebServer espServer(80);
MAX30105 particleSensor;
Servo vaultDoor;

unsigned long lastSendTime = 0;
long lastDist = -1;              // Track previous distance for the buzzer logic
unsigned long buzzerOffTime = 0; // Track when to turn the buzzer off

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  vaultDoor.attach(SERVO_PIN);
  vaultDoor.write(180); 

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 missing.");
  } else {
    particleSensor.setup(); 
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Connected!");
  Serial.print("ESP32 IP Address: "); Serial.println(WiFi.localIP());

  espServer.on("/open_door", []() {
    espServer.send(200, "text/plain", "OK");
    Serial.println("Authorized! Opening Door.");
    vaultDoor.write(90); 
    delay(2000);         
    vaultDoor.write(180); 
  });

  espServer.begin();
}

void loop() {
  espServer.handleClient();

  // Read the single ultrasonic sensor (now placed at the exit)
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  long distance = duration * 0.034 / 2;

  long irValue = particleSensor.getIR();

  // Buzz when someone is detected exiting (distance decreasing and under 100cm)
  if (lastDist > 0 && distance > 0 && distance < lastDist && distance < 100) {
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerOffTime = millis() + 1000; // Buzz for 1 second
  }
  if (millis() > buzzerOffTime) {
    digitalWrite(BUZZER_PIN, LOW);
  }
  lastDist = distance;

  if (millis() - lastSendTime > 500) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(laptopApiUrl);
      http.addHeader("Content-Type", "application/json");
      String json = "{\"distance\":" + String(distance) + ",\"ir_value\":" + String(irValue) + "}";
      http.POST(json);
      http.end();
    }
    lastSendTime = millis();
  }
}