#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Bank_Vault_Secure";
const char* password = "AdminPassword";

WebServer server(80);

// Variables to hold fake sensor data for testing
int simulatedPulse = 72;
bool vaultSecure = true;
int irCount = 0;
String proxStatus = "CLEAR";

void handleData() {
  // MUST INCLUDE THIS LINE to allow your local HTML file to read the data (CORS bypass)
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  // Create a JSON string with the sensor data
  String json = "{";
  json += "\"pulse\": " + String(simulatedPulse) + ",";
  json += "\"doorSecure\": " + String(vaultSecure ? "true" : "false") + ",";
  json += "\"occupancy\": " + String(irCount) + ",";
  json += "\"proximity\": \"" + proxStatus + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("API Active at IP: ");
  Serial.println(IP);

  // When the JS asks for /data, run the handleData function
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();
  
  // Simulate data changing slightly over time for UI testing
  simulatedPulse = random(70, 85); 
  delay(10); // Short delay to keep the loop stable
}