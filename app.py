# # from fastapi import FastAPI, Request
# # from fastapi.responses import HTMLResponse
# # from fastapi.staticfiles import StaticFiles
# # from fastapi.templating import Jinja2Templates
# # from pydantic import BaseModel
# # import sqlite3
# # import time
# # import requests

# # app = FastAPI()

# # # --- CONFIGURATION ---
# # # PUT YOUR ESP32'S IP ADDRESS HERE
# # ESP32_URL = "http://192.168.x.x/open_door" 

# # app.mount("/static", StaticFiles(directory="static"), name="static")
# # templates = Jinja2Templates(directory="templates")

# # current_distance = 0
# # ir_triggered = False
# # failed_attempts = {} 
# # lockouts = {}        

# # # --- DATABASE SETUP ---
# # def init_db():
# #     conn = sqlite3.connect('vault_data.db')
# #     c = conn.cursor()
# #     c.execute('CREATE TABLE IF NOT EXISTS clients (id TEXT PRIMARY KEY, password TEXT, vault_no TEXT)')
# #     c.execute('INSERT OR IGNORE INTO clients (id, password, vault_no) VALUES ("admin", "1234", "V-770")')
# #     c.execute('INSERT OR IGNORE INTO clients (id, password, vault_no) VALUES ("user1", "pass", "V-102")')
# #     conn.commit()
# #     conn.close()

# # init_db()

# # # --- DATA MODELS ---
# # class SensorData(BaseModel):
# #     distance: int
# #     ir_value: int

# # class LoginData(BaseModel):
# #     id: str
# #     password: str

# # # --- ROUTES ---
# # @app.get("/", response_class=HTMLResponse)
# # async def serve_ui(request: Request):
# #     return templates.TemplateResponse("index.html", {"request": request})

# # @app.post("/api/update_sensor")
# # async def update_sensor(data: SensorData):
# #     global current_distance, ir_triggered
# #     current_distance = data.distance
# #     if data.ir_value > 50000:
# #         ir_triggered = True
# #     return {"status": "ok"}

# # @app.get("/api/get_state")
# # async def get_state():
# #     return {"distance": current_distance, "ir_triggered": ir_triggered}

# # @app.post("/api/login")
# # async def login(data: LoginData):
# #     global ir_triggered
# #     current_time = time.time()
# #     client_id = data.id
    
# #     if client_id in lockouts:
# #         if current_time < lockouts[client_id]:
# #             remaining = int(lockouts[client_id] - current_time)
# #             return {"success": False, "error": f"ACCOUNT FROZEN. Try again in {remaining}s."}
# #         else:
# #             del lockouts[client_id]
# #             failed_attempts[client_id] = []

# #     conn = sqlite3.connect('vault_data.db')
# #     c = conn.cursor()
# #     c.execute('SELECT password, vault_no FROM clients WHERE id=?', (client_id,))
# #     record = c.fetchone()
# #     conn.close()

# #     if not record:
# #         return {"success": False, "error": "Invalid ID."}

# #     correct_password, vault_no = record

# #     if data.password == correct_password:
# #         failed_attempts[client_id] = []
# #         ir_triggered = False 
# #         try:
# #             requests.get(ESP32_URL, timeout=3)
# #         except:
# #             print("Warning: Could not reach ESP32.")
# #         return {"success": True, "vault_no": vault_no}
    
# #     else:
# #         if client_id not in failed_attempts:
# #             failed_attempts[client_id] = []
# #         failed_attempts[client_id].append(current_time)
# #         failed_attempts[client_id] = [t for t in failed_attempts[client_id] if current_time - t < 60]
# #         attempts_made = len(failed_attempts[client_id])
        
# #         if attempts_made >= 3:
# #             lockouts[client_id] = current_time + 60 
# #             return {"success": False, "error": "MAX ATTEMPTS REACHED. ACCOUNT FROZEN."}
# #         else:
# #             return {"success": False, "error": f"Wrong Password. {3 - attempts_made} attempts left."}

# # if __name__ == "__main__":
# #     import uvicorn
# #     uvicorn.run(app, host="0.0.0.0", port=8000)
# import os
# from fastapi import FastAPI, Request
# from fastapi.responses import HTMLResponse
# from fastapi.staticfiles import StaticFiles
# from fastapi.templating import Jinja2Templates
# from pydantic import BaseModel
# import sqlite3
# import time
# import requests

# # --- BULLETPROOF PATH RESOLUTION ---
# # This forces Python to look in the exact folder where app.py is saved
# BASE_DIR = os.path.dirname(os.path.abspath(__file__))
# TEMPLATES_DIR = os.path.join(BASE_DIR, "templates")
# STATIC_DIR = os.path.join(BASE_DIR, "static")

# # Print the paths to the terminal so we can visually verify them
# print("--- SYSTEM PATH CHECK ---")
# print(f"Base Directory: {BASE_DIR}")
# print(f"Templates Directory: {TEMPLATES_DIR}")
# print(f"Static Directory: {STATIC_DIR}")
# print("-------------------------")

# app = FastAPI()

# ESP32_URL = "http://192.168.x.x/open_door" 

# app.mount("/static", StaticFiles(directory=STATIC_DIR), name="static")
# templates = Jinja2Templates(directory=TEMPLATES_DIR)

# current_distance = 0
# ir_triggered = False
# failed_attempts = {} 
# lockouts = {}        

# def init_db():
#     db_path = os.path.join(BASE_DIR, 'vault_data.db')
#     conn = sqlite3.connect(db_path)
#     c = conn.cursor()
#     c.execute('CREATE TABLE IF NOT EXISTS clients (id TEXT PRIMARY KEY, password TEXT, vault_no TEXT)')
#     c.execute('INSERT OR IGNORE INTO clients (id, password, vault_no) VALUES ("admin", "1234", "V-770")')
#     c.execute('INSERT OR IGNORE INTO clients (id, password, vault_no) VALUES ("user1", "pass", "V-102")')
#     conn.commit()
#     conn.close()

# init_db()

# class SensorData(BaseModel):
#     distance: int
#     ir_value: int

# class LoginData(BaseModel):
#     id: str
#     password: str

# @app.get("/", response_class=HTMLResponse)
# async def serve_ui(request: Request):
#     return templates.TemplateResponse("index.html", {"request": request})

# @app.post("/api/update_sensor")
# async def update_sensor(data: SensorData):
#     global current_distance, ir_triggered
#     current_distance = data.distance
#     if data.ir_value > 50000:
#         ir_triggered = True
#     return {"status": "ok"}

# @app.get("/api/get_state")
# async def get_state():
#     return {"distance": current_distance, "ir_triggered": ir_triggered}

# @app.post("/api/login")
# async def login(data: LoginData):
#     global ir_triggered
#     current_time = time.time()
#     client_id = data.id
    
#     if client_id in lockouts:
#         if current_time < lockouts[client_id]:
#             remaining = int(lockouts[client_id] - current_time)
#             return {"success": False, "error": f"ACCOUNT FROZEN. Try again in {remaining}s."}
#         else:
#             del lockouts[client_id]
#             failed_attempts[client_id] = []

#     db_path = os.path.join(BASE_DIR, 'vault_data.db')
#     conn = sqlite3.connect(db_path)
#     c = conn.cursor()
#     c.execute('SELECT password, vault_no FROM clients WHERE id=?', (client_id,))
#     record = c.fetchone()
#     conn.close()

#     if not record:
#         return {"success": False, "error": "Invalid ID."}

#     correct_password, vault_no = record

#     if data.password == correct_password:
#         failed_attempts[client_id] = []
#         ir_triggered = False 
#         try:
#             requests.get(ESP32_URL, timeout=3)
#         except:
#             print("Warning: Could not reach ESP32.")
#         return {"success": True, "vault_no": vault_no}
    
#     else:
#         if client_id not in failed_attempts:
#             failed_attempts[client_id] = []
#         failed_attempts[client_id].append(current_time)
#         failed_attempts[client_id] = [t for t in failed_attempts[client_id] if current_time - t < 60]
#         attempts_made = len(failed_attempts[client_id])
        
#         if attempts_made >= 3:
#             lockouts[client_id] = current_time + 60 
#             return {"success": False, "error": "MAX ATTEMPTS REACHED. ACCOUNT FROZEN."}
#         else:
#             return {"success": False, "error": f"Wrong Password. {3 - attempts_made} attempts left."}

# if __name__ == "__main__":
#     import uvicorn
#     uvicorn.run(app, host="0.0.0.0", port=8000)
import os
from fastapi import FastAPI, Request
from fastapi.responses import HTMLResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
from pydantic import BaseModel
import sqlite3
import time
import requests

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

def init_db():
    db_path = os.path.join(BASE_DIR, 'vault_data.db')
    conn = sqlite3.connect(db_path)
    c = conn.cursor()
    c.execute('CREATE TABLE IF NOT EXISTS clients (id TEXT PRIMARY KEY, password TEXT, vault_no TEXT)')
    c.execute('INSERT OR IGNORE INTO clients (id, password, vault_no) VALUES ("admin", "1234", "V-770")')
    c.execute('INSERT OR IGNORE INTO clients (id, password, vault_no) VALUES ("user1", "pass", "V-102")')
    conn.commit()
    conn.close()

init_db()

class SensorData(BaseModel):
    distance: int
    ir_value: int

class LoginData(BaseModel):
    id: str
    password: str

@app.get("/", response_class=HTMLResponse)
async def serve_ui(request: Request):
    return templates.TemplateResponse(request=request, name="index.html") #FIX1: new Starlette API - pass request as keyword arg, not inside dict

@app.post("/api/update_sensor")
async def update_sensor(data: SensorData):
    global current_distance, ir_triggered
    current_distance = data.distance
    if data.ir_value > 50000:
        ir_triggered = True
    return {"status": "ok"}

@app.get("/api/get_state")
async def get_state():
    return {"distance": current_distance, "ir_triggered": ir_triggered}

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
            print("Warning: Could not reach ESP32.")
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