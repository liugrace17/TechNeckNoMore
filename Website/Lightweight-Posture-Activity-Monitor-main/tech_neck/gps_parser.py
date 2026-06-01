import serial
import struct
import threading
import time

# ---------------------------------------------------------------------------
# Serial settings
# ---------------------------------------------------------------------------

SERIAL_PORT = "/dev/serial0"
BAUD_RATE = 115200

# Must match ESP32 GpsPacket:
# startByte: uint8_t
# latitude: float
# longitude: float
# valid: uint8_t
# checksum: uint8_t
PACKET_SIZE = 11
START_BYTE = 0xAA

# ---------------------------------------------------------------------------
# Shared GPS state for FastAPI
# ---------------------------------------------------------------------------

lock = threading.Lock()

latest_lat = 0.0
latest_lng = 0.0
latest_valid = 0

# List of valid GPS points for /gps/route
route = []


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def checksum(data: bytes) -> int:
    cs = 0
    for b in data:
        cs ^= b
    return cs


# ---------------------------------------------------------------------------
# Main GPS reader
# ---------------------------------------------------------------------------

def run():
    global latest_lat, latest_lng, latest_valid, route

    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

    print("GPS parser started", flush=True)

    while True:
        # Wait for start byte
        b = ser.read(1)

        if not b:
            continue

        if b[0] != START_BYTE:
            continue

        # Read the rest of the packet
        rest = ser.read(PACKET_SIZE - 1)

        if len(rest) != PACKET_SIZE - 1:
            continue

        packet = b + rest

        received_checksum = packet[-1]
        calculated_checksum = checksum(packet[:-1])

        if received_checksum != calculated_checksum:
            print("Bad GPS checksum", flush=True)
            continue

        # ESP32 sends little-endian:
        # uint8_t, float, float, uint8_t, uint8_t
        start, lat, lng, valid, cs = struct.unpack("<BffBB", packet)

        with lock:
            latest_lat = lat
            latest_lng = lng
            latest_valid = valid

            # Only add real GPS fixes to route
            if valid == 1:
                route.append({"lat": lat, "lng": lng})

                # Prevent route from growing forever
                if len(route) > 500:
                    route = route[-500:]

        print(
            f"GPS packet: valid={valid}, lat={lat:.6f}, lng={lng:.6f}",
            flush=True
        )


# ---------------------------------------------------------------------------
# Allow direct testing with: python3 gps_parser.py
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    run()