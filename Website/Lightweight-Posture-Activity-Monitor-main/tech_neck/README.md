# TechNeck No More

A cross-platform Flutter application for real-time posture monitoring and activity tracking, built as part of a wearable health device project. The app interfaces with a Raspberry Pi 4 running a custom BLE peripheral and FastAPI backend to display live sensor data from an ESP32/STM32-based wearable vest.

## Project Structure

```
lib/
├── main.dart
├── models/models.dart
├── services/
│   ├── api_service.dart              # HTTP calls to FastAPI (web)
│   ├── ble_service.dart              # BLE central — connects to Pi (mobile)
│   ├── stub_ble_service.dart         # Web no-op stub
│   ├── location_service.dart         # Orchestrates BLE vs phone GPS
│   ├── phone_location_service.dart   # Geolocator fallback
│   └── stub_phone_location_service.dart
├── theme/app_theme.dart
├── screens/dashboard_screen.dart
└── widgets/
    ├── stat_card.dart
    ├── posture_chart.dart
    ├── route_map.dart
    └── activity_indicator.dart
```


## Pi Setup

### Dependencies

```bash
pip install fastapi uvicorn bluezero
```

### Run

```bash
sudo -E env PATH=$PATH python ble.py
```

Requires `sudo` for BLE peripheral access. The server starts both the BLE peripheral (port: BLE) and FastAPI (port: 8000) simultaneously.

### BLE Payload Format

The Pi broadcasts a single GATT characteristic with the following format:

```
"steps,active_minutes,posture_pct,current_activity,idle_streak,lat,lng"
```

`lat` and `lng` are `null` when no GPS fix is available — the mobile app falls back to phone GPS automatically.

---

## Flutter Setup

### Dependencies

```bash
flutter pub get
```

### Run (Web)

```bash
flutter run -d chrome --web-browser-flag="--disable-web-security"
```

### Run (Android)

```bash
flutter run -d <device-id>
```