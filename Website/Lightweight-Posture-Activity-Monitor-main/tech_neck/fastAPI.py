"""
FastAPI server — reads live state from bt_parser.py and gps_parser.py
Run with: uvicorn fastAPI:app --host 0.0.0.0 --port 8000
"""

import threading
import bt_parser
import gps_parser

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
# Start Bluetooth + GPS listeners on startup
# ---------------------------------------------------------------------------

@app.on_event("startup")
def start_bt():
    t = threading.Thread(target=bt_parser.run, daemon=True)
    t.start()

    gps_t = threading.Thread(target=gps_parser.run, daemon=True)
    gps_t.start()

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

class GpsStatus(BaseModel):
    valid: int
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
            active_minutes=bt_parser.active_time,
            posture_goal_percentage=bt_parser.posture_goal_percentage,
            current_activity=bt_parser.current_activity,
            idle_streak_minutes=bt_parser.idle_streak,
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
        # If we have a real GPS route, use it
        if len(gps_parser.route) > 0:
            return [GpsPoint(lat=p["lat"], lng=p["lng"]) for p in gps_parser.route]

        # If route has not built yet but latest GPS is valid, show latest point
        if gps_parser.latest_valid == 1:
            return [
                GpsPoint(
                    lat=gps_parser.latest_lat,
                    lng=gps_parser.latest_lng,
                )
            ]

        # If GPS has no fix yet, return empty route
        return []