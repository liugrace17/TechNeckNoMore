"""
FastAPI server — reads live state from bt_parser.py
Run with: uvicorn fastAPI:app --host 0.0.0.0 --port 8000
"""

import threading
import app

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from typing import List

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# ---------------------------------------------------------------------------
# Start Bluetooth listener on startup
# ---------------------------------------------------------------------------

@app.on_event("startup")
def start_bt():
    t = threading.Thread(target=bt_parser.run, daemon=True)
    t.start()

# ---------------------------------------------------------------------------
# Models
# ---------------------------------------------------------------------------

class ActivitySummary(BaseModel):
    steps: int
    active_minutes: int
    posture_goal_percentage: float
    current_activity: str
    idle_streak_minutes: int

class GpsPoint(BaseModel):
    lat: float
    lng: float

# ---------------------------------------------------------------------------
# Routes
# ---------------------------------------------------------------------------

@app.get("/activity/summary", response_model=ActivitySummary)
def get_activity_summary():
    with bt_parser.lock:
        return ActivitySummary(
            steps=bt_parser.steps,
            active_minutes=bt_parser.active_time,  # bt_parser uses active_time internally
            posture_goal_percentage=bt_parser.posture_goal_percentage,
            current_activity=bt_parser.current_activity,
            idle_streak_minutes=bt_parser.idle_streak,
        )

@app.get("/gps/route", response_model=List[GpsPoint])
def get_gps_route():
    # TODO: replace with live GPS data when available
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
