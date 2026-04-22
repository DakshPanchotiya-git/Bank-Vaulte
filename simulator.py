import requests
import time
import threading

# Pointing to your local FastAPI server
API_URL = "http://127.0.0.1:8000/api/update_sensor"

# Starting sensor states
current_distance = 200 
ir_value = 1000 

def simulate_esp32():
    global current_distance, ir_value
    while True:
        try:
            # Send the fake data to FastAPI
            payload = {"distance": current_distance, "ir_value": ir_value}
            requests.post(API_URL, json=payload)
            
            # Make the distance slowly decrease to simulate someone walking closer
            current_distance -= 5
            if current_distance < 10: 
                current_distance = 200
                
        except requests.exceptions.ConnectionError:
            pass # Ignore errors if FastAPI isn't running yet
            
        time.sleep(0.5) # Send every 500ms, exactly like the Arduino code

# Run the background loop
threading.Thread(target=simulate_esp32, daemon=True).start()

print("--- ESP32 HARDWARE SIMULATOR ---")
print("Simulating distance data in the background...")
print("Press ENTER at any time to simulate placing a finger on the scanner.")

while True:
    input() # Wait for you to press Enter in the terminal
    print(">> FINGER DETECTED! Sending IR > 50000...")
    ir_value = 60000 # Spike the IR value
    time.sleep(1)    # Hold it for 1 second
    ir_value = 1000  # Remove the fake finger