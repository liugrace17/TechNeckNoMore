"""
Raspberry Pi Mock FastAPI Server
Run with: pip install fastapi uvicorn
Then: uvicorn pi_server:app --reload
"""

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
# Models
# ---------------------------------------------------------------------------

class ActivitySummary(BaseModel):
    steps: int
    standing_minutes: int
    posture_goal_percentage: float

class GpsPoint(BaseModel):
    lat: float
    lng: float

# ---------------------------------------------------------------------------
# Hardcoded mock data
# ---------------------------------------------------------------------------

SUMMARY = ActivitySummary(
    steps=8432,
    standing_minutes=214,
    posture_goal_percentage=68.5,
)

GPS_ROUTE = [
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

# ---------------------------------------------------------------------------
# Routes
# ---------------------------------------------------------------------------

@app.get("/activity/summary", response_model=ActivitySummary)
def get_activity_summary():
    return SUMMARY

@app.get("/gps/route", response_model=List[GpsPoint])
def get_gps_route():
    return GPS_ROUTE