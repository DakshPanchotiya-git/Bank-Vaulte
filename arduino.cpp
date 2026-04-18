#include <WiFi.h>
#include <WebServer.h>

// 1. Setup your Access Point credentials
const char* ssid = "Bank_Vault_Secure";
const char* password = "AdminPassword"; // Standard minimum 8 characters

// 2. Start the web server on port 80
WebServer server(80);

// 3. The Real-World Dashboard UI (HTML + Advanced CSS)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Vault Security Interface</title>
  <style>
    /* High-Tech Security Theme */
    :root { 
      --bg: #050914; 
      --panel: #0d1527; 
      --accent-green: #00ff88; 
      --accent-red: #ff3333; 
      --text-main: #e2e8f0; 
      --border-color: #1e293b;
    }
    
    body { 
      margin: 0; 
      padding: 20px; 
      font-family: 'Consolas', 'Courier New', monospace; 
      background-color: var(--bg); 
      color: var(--text-main); 
    }

    /* Main Header Styling */
    .header { 
      text-align: center; 
      border-bottom: 1px solid var(--border-color); 
      padding-bottom: 20px; 
      margin-bottom: 25px; 
    }
    
    .header h1 { 
      margin: 0; 
      font-size: 1.4rem; 
      letter-spacing: 4px; 
      color: #ffffff; 
      font-weight: normal;
    }

    /* Pulsing Armed Badge */
    .status-badge { 
      display: inline-block; 
      margin-top: 12px; 
      padding: 6px 18px; 
      background: rgba(0, 255, 136, 0.05); 
      border: 1px solid var(--accent-green); 
      color: var(--accent-green); 
      border-radius: 4px; 
      font-weight: bold; 
      font-size: 0.85rem; 
      letter-spacing: 2px;
      box-shadow: 0 0 12px rgba(0, 255, 136, 0.2); 
      animation: pulse 2s infinite; 
    }

    @keyframes pulse { 
      0% { box-shadow: 0 0 5px rgba(0, 255, 136, 0.2); } 
      50% { box-shadow: 0 0 20px rgba(0, 255, 136, 0.6); } 
      100% { box-shadow: 0 0 5px rgba(0, 255, 136, 0.2); } 
    }

    /* Grid Layout for Sensor Modules */
    .grid { 
      display: grid; 
      grid-template-columns: 1fr 1fr; 
      gap: 15px; 
    }

    /* Individual Sensor Panels */
    .card { 
      background: var(--panel); 
      border: 1px solid var(--border-color); 
      border-radius: 6px; 
      padding: 15px; 
      display: flex; 
      flex-direction: column; 
      align-items: center; 
      justify-content: center; 
      text-align: center; 
      position: relative; 
      overflow: hidden; 
      box-shadow: inset 0 0 20px rgba(0,0,0,0.5);
    }

    /* Top Indicator Bar on Panels */
    .card::before { 
      content: ''; 
      position: absolute; 
      top: 0; 
      left: 0; 
      width: 100%; 
      height: 3px; 
    }
    .card.secure::before { background: var(--accent-green); }
    .card.alert::before { background: var(--accent-red); }

    .card-title { 
      font-size: 0.75rem; 
      color: #64748b; 
      text-transform: uppercase; 
      letter-spacing: 1px; 
      margin-bottom: 8px; 
    }

    .card-value { 
      font-size: 1.5rem; 
      font-weight: bold; 
    }
    
    .value-green { color: var(--accent-green); }
    .value-red { color: var(--accent-red); }
    .value-neutral { color: #94a3b8; }
  </style>
</head>
<body>

  <div class="header">
    <h1>CENTRAL VAULT SYSTEM</h1>
    <div class="status-badge">SYSTEM ARMED</div>
  </div>

  <div class="grid">
    <div class="card secure">
      <div class="card-title">BIOMETRIC (PULSE)</div>
      <div class="card-value value-neutral" id="val-pulse">-- BPM</div>
    </div>
    
    <div class="card secure">
      <div class="card-title">VAULT DOOR (LDR)</div>
      <div class="card-value value-green" id="val-ldr">SECURE</div>
    </div>
    
    <div class="card secure">
      <div class="card-title">PROXIMITY (ULTRA)</div>
      <div class="card-value value-green" id="val-dist">CLEAR</div>
    </div>
    
    <div class="card secure">
      <div class="card-title">OCCUPANCY (IR)</div>
      <div class="card-value value-green" id="val-ir">0</div>
    </div>
  </div>

</body>
</html>
)rawliteral";

void handleRoot() {
  // Serve the dashboard when a client connects
  server.send(200, "text/html", index_html);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting Vault Mainframe...");

  // Start the Access Point
  WiFi.softAP(ssid, password);
  
  // Print the local IP
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Network Active. UI available at: ");
  Serial.println(IP);

  // Route for the root web page
  server.on("/", handleRoot);
  
  // Start the web server
  server.begin();
  Serial.println("HTTP Server Online.");
}

void loop() {
  // Listen for the Kodular App requesting the page
  server.handleClient();
}