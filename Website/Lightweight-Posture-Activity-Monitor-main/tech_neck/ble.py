"""
FastAPI server — reads live state from uart_parser.py
Run with: python ble.py
"""

import threading
import logging
import uvicorn
import uart_parser
import gps_parser
from bluezero import peripheral, adapter, async_tools
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
    with uart_parser.lock:
        return (
            f"{uart_parser.steps},"
            f"{uart_parser.active_time},"
            f"{uart_parser.posture_goal_percentage},"
            f"{uart_parser.current_activity},"
            f"{uart_parser.idle_streak},"
            f"{gps_parser.latest_lat},"
            f"{gps_parser.latest_lng}"
        )

def read_value():
    return encode(payload_string())

def update_value(characteristic):
    characteristic.set_value(encode(payload_string()))
    logger.info(f"[BLE] pushed: {payload_string()}")
    return True

def notify_callback(notifying, characteristic):
    if notifying:
        async_tools.add_timer_seconds(1, update_value, characteristic)

def main_ble():
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
        value=[], notifying=False,
        flags=['read', 'notify'],
        read_callback=read_value,
        write_callback=None,
        notify_callback=notify_callback,
    )

    pi_peripheral.publish()
    logger.info("BLE peripheral running...")

# ---------------------------------------------------------------------------
# FastAPI
# ---------------------------------------------------------------------------

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

class GpsStatus(BaseModel):
    valid: int
    lat: float
    lng: float

@app.on_event("startup")
def start_bt():
    threading.Thread(target=uart_parser.run, daemon=True).start()

@app.get("/activity/summary", response_model=ActivitySummary)
def get_activity_summary():
    with uart_parser.lock:
        return ActivitySummary(
            steps=uart_parser.steps,
            active_minutes=uart_parser.active_time,
            posture_goal_percentage=uart_parser.posture_goal_percentage,
            current_activity=uart_parser.current_activity,
            idle_streak_minutes=uart_parser.idle_streak,
        )

@app.get("/gps/latest", response_model=GpsStatus)
def get_latest_gps():
    with gps_parser.lock:
        return GpsStatus(
            valid=gps_parser.latest_valid,
            lat=gps_parser.latest_lat,
            lng=gps_parser.latest_lng,
        )

@app.get("/gps/route", response_model=List[GpsPoint])
def get_gps_route():
    with gps_parser.lock:
        if len(gps_parser.route) > 0:
            return [GpsPoint(lat=p["lat"], lng=p["lng"]) for p in gps_parser.route]
        return [GpsPoint(lat=gps_parser.latest_lat, lng=gps_parser.latest_lng)]

if __name__ == "__main__":
    threading.Thread(target=lambda: uvicorn.run(app, host="0.0.0.0", port=8000), daemon=True).start()
    main_ble()