import serial
import subprocess
import time
import os 
import threading
import struct
from collections import namedtuple

HC05_MAC = '00:23:00:00:AA:BA'
BAUD_RATE = 9600

lock = threading.Lock()

steps = 0
active_time = 0
idle_minutes = 0
idle_streak = 0
posture_goal_percentage = 0.0
current_activity = "idle"

ACTIVITY_MAP = {
    3: "standing",
    4: "idle",
    6: "active",
    7: "active",
    8: "active"
}

ImuMsg = namedtuple('ImuMsg', [
    'start', 'goodPos', 'activityType',
    'idleTime', 'activeTime', 'goodPosCount', 'badPosCount',
    'steps', 'checksum'
])

def _setup_bluetooth():
    print("Setting up Bluetooth...")
    subprocess.run(['sudo', 'rfcomm', 'release', 'all'], capture_output=True)
    time.sleep(1)

    proc = subprocess.Popen(
        ['sudo', 'rfcomm', 'connect', 'rfcomm0', HC05_MAC, '1'],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )

    start = time.time()
    while time.time() - start < 10:
        if os.path.exists('/dev/rfcomm0'):
            print("Bluetooth connected")
            return proc
        time.sleep(0.5)

    print("Bluetooth connection timeout")
    proc.terminate()
    return None

def _parse_loop(ser):
    global steps, active_time, idle_streak
    global posture_goal_percentage, current_activity

    imu_start = b'\xFF'

    while True:
        try:
            if ser.in_waiting > 0:
                byte = ser.read()
                if byte == imu_start:
                    raw = byte + ser.read(21)
                    msg = ImuMsg._make(struct.unpack('B ? B I I I I H B', raw))

                    total = msg.goodPosCount + msg.badPosCount
                    posture_pct = (msg.goodPosCount / total * 100.0) if total > 0 else 0.0

                    with lock:
                        steps                   = msg.steps
                        active_time             = msg.activeTime // 60000
                        idle_streak             = msg.idleTime // 60000
                        posture_goal_percentage = posture_pct
                        current_activity        = ACTIVITY_MAP.get(msg.activityType, "idle")

        except Exception as e:
            print(f"Parse error: {e}")
            time.sleep(0.1)
            break

def run():
    while True:
        bt_proc = _setup_bluetooth()
        if bt_proc is None:
            print("Bluetooth setup failed — state will remain at defaults")
            time.sleep(1)
            continue

        try:
            ser = serial.Serial('/dev/rfcomm0', BAUD_RATE, timeout=1)
            print("Serial port opened — listening for IMU data")
            _parse_loop(ser)
        except Exception as e:
            print(f"Serial error: {e}")
            bt_proc.terminate()
            time.sleep(1)
