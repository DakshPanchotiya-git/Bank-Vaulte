const esp32IP = 'http://192.168.4.1/data';

// Grab UI elements
const connStatus = document.getElementById('conn-status');
const valPulse = document.getElementById('val-pulse');
const valDoor = document.getElementById('val-door');
const valDist = document.getElementById('val-dist');
const valIr = document.getElementById('val-ir');
const cardDoor = document.getElementById('card-door');

async function fetchVaultData() {
    try {
        const response = await fetch(esp32IP);
        const data = await response.json();

        // Update Connection Badge
        connStatus.innerText = "SYSTEM ARMED";
        connStatus.className = "status-badge badge-online";

        // Update UI with JSON data
        valPulse.innerText = data.pulse + " BPM";
        valIr.innerText = data.occupancy;
        valDist.innerText = data.proximity;

        // Dynamic Alert Logic for the Door
        if (data.doorSecure) {
            valDoor.innerText = "SECURE";
            valDoor.className = "card-value value-green";
            cardDoor.className = "card secure";
        } else {
            valDoor.innerText = "BREACH";
            valDoor.className = "card-value value-red";
            cardDoor.className = "card alert";
        }

    } catch (error) {
        console.error("Connection lost:", error);
        connStatus.innerText = "LINK LOST";
        connStatus.className = "status-badge badge-offline";
    }
}

// Ping the ESP32 every 1 second
setInterval(fetchVaultData, 1000);