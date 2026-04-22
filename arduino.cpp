// #include <WiFi.h>
// #include <WebServer.h>
// #include <HTTPClient.h>
// #include <Wire.h>
// #include <ESP32Servo.h>
// #include "MAX30105.h"

// const char* ssid = "YOUR_WIFI_SSID";
// const char* password = "YOUR_WIFI_PASSWORD";

// // PUT YOUR LAPTOP'S IP ADDRESS HERE
// const char* laptopApiUrl = "http://192.168.x.x:8000/api/update_sensor"; 

// #define TRIG_PIN 13
// #define ECHO_PIN 25
// #define SERVO_PIN 14

// WebServer espServer(80);
// MAX30105 particleSensor;
// Servo vaultDoor;
// unsigned long lastSendTime = 0;

// void setup() {
//   Serial.begin(115200);
//   pinMode(TRIG_PIN, OUTPUT);
//   pinMode(ECHO_PIN, INPUT);

//   vaultDoor.attach(SERVO_PIN);
//   vaultDoor.write(180); 

//   if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
//     Serial.println("MAX30105 missing.");
//   } else {
//     particleSensor.setup(); 
//   }

//   WiFi.begin(ssid, password);
//   while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
//   Serial.println("\nWiFi Connected!");
//   Serial.print("ESP32 IP Address: "); Serial.println(WiFi.localIP());

//   espServer.on("/open_door", []() {
//     espServer.send(200, "text/plain", "OK");
//     Serial.println("Authorized! Opening Door.");
//     vaultDoor.write(90); 
//     delay(2000);         
//     vaultDoor.write(180); 
//   });

//   espServer.begin();
// }

// void loop() {
//   espServer.handleClient();

//   digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
//   digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
//   digitalWrite(TRIG_PIN, LOW);
//   long duration = pulseIn(ECHO_PIN, HIGH, 30000);
//   long distance = duration * 0.034 / 2;

//   long irValue = particleSensor.getIR();

//   if (millis() - lastSendTime > 500) {
//     if (WiFi.status() == WL_CONNECTED) {
//       HTTPClient http;
//       http.begin(laptopApiUrl);
//       http.addHeader("Content-Type", "application/json");
//       String json = "{\"distance\":" + String(distance) + ",\"ir_value\":" + String(irValue) + "}";
//       http.POST(json);
//       http.end();
//     }
//     lastSendTime = millis();
//   }
// }
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
#define EXIT_TRIG_PIN 26   //FIX1: exit-side ultrasonic trigger pin
#define EXIT_ECHO_PIN 27   //FIX2: exit-side ultrasonic echo pin
#define BUZZER_PIN 32      //FIX3: buzzer pin for exit detection alert

WebServer espServer(80);
MAX30105 particleSensor;
Servo vaultDoor;
unsigned long lastSendTime = 0;
long lastExitDist = -1;          //FIX4: track previous exit sensor reading
unsigned long buzzerOffTime = 0; //FIX5: when to turn buzzer off

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(EXIT_TRIG_PIN, OUTPUT);  //FIX6: exit ultrasonic trig as output
  pinMode(EXIT_ECHO_PIN, INPUT);   //FIX7: exit ultrasonic echo as input
  pinMode(BUZZER_PIN, OUTPUT);     //FIX8: buzzer as output
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

  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  long distance = duration * 0.034 / 2;

  long irValue = particleSensor.getIR();

  // --- EXIT SENSOR: read distance on exit side ---  //FIX9: exit ultrasonic logic
  digitalWrite(EXIT_TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(EXIT_TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(EXIT_TRIG_PIN, LOW);
  long exitDuration = pulseIn(EXIT_ECHO_PIN, HIGH, 30000);
  long exitDistance = exitDuration * 0.034 / 2;

  // Buzz when someone is detected exiting (distance decreasing and within range)  //FIX10: buzzer on exit
  if (lastExitDist > 0 && exitDistance > 0 && exitDistance < lastExitDist && exitDistance < 100) {
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerOffTime = millis() + 1000; // buzz for 1 second
  }
  if (millis() > buzzerOffTime) {
    digitalWrite(BUZZER_PIN, LOW);
  }
  lastExitDist = exitDistance;

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