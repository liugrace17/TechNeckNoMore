"""
FastAPI server — reads live state from bt_parser.py
Run with: python ble.py
"""

import time
import threading
import logging
import uvicorn
import bt_parser
from bluezero import peripheral, adapter
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from typing import List
 
SERVICE_UUID = "66bffa4d-fdb1-4a44-9fcb-b19fa257b833"
CHAR_UUID    = "dbbcc4ab-0707-442b-a572-fbfdc5e9ebed"
 
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)
 
def encode(value: str) -> list:
    return list(value.encode('utf-8'))
 
def payload_string() -> str:
    with bt_parser.lock:
        return (
            f"{bt_parser.steps},"
            f"{bt_parser.active_time},"
            f"{bt_parser.posture_goal_percentage},"
            f"null,null"
        )
 
def run_ble():
    ble = adapter.Adapter()
    ble.powered = True
 
    pi_peripheral = peripheral.Peripheral(
        ble.address,
        local_name='TechNeckPi',
        appearance=0x0000,
    )
 
    pi_peripheral.add_service(srv_id=1, uuid=SERVICE_UUID, primary=True)
 
    pi_peripheral.add_characteristic(
        srv_id=1, chr_id=1, uuid=CHAR_UUID,
        value=encode(payload_string()), notifying=False,
        flags=['read', 'notify'],
        read_callback=lambda: encode(payload_string()),
        write_callback=None, notify_callback=None,
    )
 
    pi_peripheral.publish()
    logger.info("BLE peripheral running...")
 
    while True:
        try:
            pi_peripheral.update_characteristic(1, 1, encode(payload_string()))
        except Exception:
            pass
        time.sleep(5)
 
app = FastAPI()
 
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)
 
class ActivitySummary(BaseModel):
    steps: int
    active_minutes: int
    posture_goal_percentage: float
    current_activity: str
    idle_streak_minutes: int
 
class GpsPoint(BaseModel):
    lat: float
    lng: float
 
@app.on_event("startup")
def start_bt():
    threading.Thread(target=bt_parser.run, daemon=True).start()
 
@app.get("/activity/summary", response_model=ActivitySummary)
def get_activity_summary():
    with bt_parser.lock:
        return ActivitySummary(
            steps=bt_parser.steps,
            active_minutes=bt_parser.active_time,
            posture_goal_percentage=bt_parser.posture_goal_percentage,
            current_activity=bt_parser.current_activity,
            idle_streak_minutes=bt_parser.idle_streak,
        )
 
@app.get("/gps/route", response_model=List[GpsPoint])
def get_gps_route():
    return [
        GpsPoint(lat=46.8698, lng=-122.2609),
        GpsPoint(lat=46.8712, lng=-122.2615),
        GpsPoint(lat=46.8725, lng=-122.2601),
        GpsPoint(lat=46.8731, lng=-122.2588),
        GpsPoint(lat=46.8740, lng=-122.2572),
        GpsPoint(lat=46.8748, lng=-122.2560),
        GpsPoint(lat=46.8741, lng=-122.2545),
        GpsPoint(lat=46.8730, lng=-122.2538),
        GpsPoint(lat=46.8718, lng=-122.2542),
        GpsPoint(lat=46.8705, lng=-122.2551),
        GpsPoint(lat=46.8698, lng=-122.2565),
        GpsPoint(lat=46.8694, lng=-122.2580),
        GpsPoint(lat=46.8698, lng=-122.2609),
    ]
 
if __name__ == "__main__":
    threading.Thread(target=run_ble, daemon=True).start()
    uvicorn.run(app, host="0.0.0.0", port=8000)
