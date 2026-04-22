import os
from fastapi import FastAPI, Request
from fastapi.responses import HTMLResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
from pydantic import BaseModel
import sqlite3
import time
import requests
import random

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
TEMPLATES_DIR = os.path.join(BASE_DIR, "templates")
STATIC_DIR = os.path.join(BASE_DIR, "static")

app = FastAPI()

ESP32_URL = "http://192.168.x.x/open_door"

app.mount("/static", StaticFiles(directory=STATIC_DIR), name="static")
templates = Jinja2Templates(directory=TEMPLATES_DIR)

current_distance = 0
ir_triggered = False
failed_attempts = {}
lockouts = {}
last_sensor_update = 0 # Tracks when the ESP32 last sent data

def init_db():
    db_path = os.path.join(BASE_DIR, 'vault_data.db')
    conn = sqlite3.connect(db_path)
    c = conn.cursor()
    # Added 'name' to the table schema
    c.execute('CREATE TABLE IF NOT EXISTS clients (id TEXT PRIMARY KEY, name TEXT, password TEXT, vault_no TEXT)')
    # Default test user
    c.execute('INSERT OR IGNORE INTO clients (id, name, password, vault_no) VALUES ("admin", "System Admin", "1234", "V-770")')
    conn.commit()
    conn.close()

init_db()

class SensorData(BaseModel):
    distance: int
    ir_value: int

class LoginData(BaseModel):
    id: str
    password: str

class RegisterData(BaseModel):
    name: str
    id: str
    password: str

@app.get("/", response_class=HTMLResponse)
async def serve_ui(request: Request):
    return templates.TemplateResponse(request=request, name="index.html") 

@app.post("/api/update_sensor")
async def update_sensor(data: SensorData):
    global current_distance, ir_triggered, last_sensor_update
    last_sensor_update = time.time() # Refresh the health timer
    current_distance = data.distance
    if data.ir_value > 50000:
        ir_triggered = True
    return {"status": "ok"}

@app.get("/api/get_state")
async def get_state():
    # If we received data in the last 3 seconds, sensors are alive
    sensors_alive = (time.time() - last_sensor_update) < 3.0
    return {
        "distance": current_distance, 
        "ir_triggered": ir_triggered,
        "sensors_alive": sensors_alive
    }

@app.post("/api/register")
async def register(data: RegisterData):
    db_path = os.path.join(BASE_DIR, 'vault_data.db')
    conn = sqlite3.connect(db_path)
    c = conn.cursor()
    
    # Check if user ID already exists
    c.execute('SELECT id FROM clients WHERE id=?', (data.id,))
    if c.fetchone():
        conn.close()
        return {"success": False, "error": "USER ID ALREADY EXISTS."}
    
    # Generate random vault number
    vault_no = f"V-{random.randint(100, 999)}"
    
    # Insert new client
    c.execute('INSERT INTO clients (id, name, password, vault_no) VALUES (?, ?, ?, ?)', 
              (data.id, data.name, data.password, vault_no))
    conn.commit()
    conn.close()
    
    return {"success": True, "vault_no": vault_no}

@app.post("/api/login")
async def login(data: LoginData):
    global ir_triggered
    current_time = time.time()
    client_id = data.id

    if client_id in lockouts:
        if current_time < lockouts[client_id]:
            remaining = int(lockouts[client_id] - current_time)
            return {"success": False, "error": f"ACCOUNT FROZEN. Try again in {remaining}s."}
        else:
            del lockouts[client_id]
            failed_attempts[client_id] = []

    db_path = os.path.join(BASE_DIR, 'vault_data.db')
    conn = sqlite3.connect(db_path)
    c = conn.cursor()
    c.execute('SELECT password, vault_no FROM clients WHERE id=?', (client_id,))
    record = c.fetchone()
    conn.close()

    if not record:
        return {"success": False, "error": "Invalid ID."}

    correct_password, vault_no = record

    if data.password == correct_password:
        failed_attempts[client_id] = []
        ir_triggered = False
        try:
            requests.get(ESP32_URL, timeout=3)
        except:
            pass
        return {"success": True, "vault_no": vault_no}

    else:
        if client_id not in failed_attempts:
            failed_attempts[client_id] = []
        failed_attempts[client_id].append(current_time)
        failed_attempts[client_id] = [t for t in failed_attempts[client_id] if current_time - t < 60]
        attempts_made = len(failed_attempts[client_id])

        if attempts_made >= 3:
            lockouts[client_id] = current_time + 60
            return {"success": False, "error": "MAX ATTEMPTS REACHED. ACCOUNT FROZEN."}
        else:
            return {"success": False, "error": f"Wrong Password. {3 - attempts_made} attempts left."}

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)