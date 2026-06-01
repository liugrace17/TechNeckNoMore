import 'dart:async';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:latlong2/latlong.dart';

class PiPayload {
  final int steps;
  final int activeMinutes;
  final double postureGoalPercentage;
  final String currentActivity;
  final int idleStreakMinutes;
  final double? lat;
  final double? lng;

  PiPayload({
    required this.steps,
    required this.activeMinutes,
    required this.postureGoalPercentage,
    required this.currentActivity,
    required this.idleStreakMinutes,
    this.lat,
    this.lng,
  });

  bool get hasGps => lat != null && lng != null;
  LatLng? get position => hasGps ? LatLng(lat!, lng!) : null;

  // Format: "steps,active_minutes,posture_pct,current_activity,idle_streak,lat,lng"
  factory PiPayload.fromString(String raw) {
    final p = raw.trim().split(',');
    if (p.length != 7) return PiPayload(steps: 0, activeMinutes: 0, postureGoalPercentage: 0, currentActivity: 'idle', idleStreakMinutes: 0);
    return PiPayload(
      steps:                 int.tryParse(p[0]) ?? 0,
      activeMinutes:         int.tryParse(p[1]) ?? 0,
      postureGoalPercentage: double.tryParse(p[2]) ?? 0,
      currentActivity:       p[3],
      idleStreakMinutes:      int.tryParse(p[4]) ?? 0,
      lat:                   double.tryParse(p[5]),
      lng:                   double.tryParse(p[6]),
    );
  }
}

class BleService {
  static const String _serviceUuid = "12345678-1234-1234-1234-123456789abc";
  static const String _charUuid    = "12345678-1234-1234-1234-123456789def";

  static final _payloadController = StreamController<PiPayload>.broadcast();
  static Stream<PiPayload> get stream => _payloadController.stream;

  static StreamSubscription? _scanSub;
  static StreamSubscription? _notifySub;
  static BluetoothDevice?    _device;

  static Future<void> start() async {
    await FlutterBluePlus.startScan(
      withServices: [Guid(_serviceUuid)],
      timeout: const Duration(seconds: 30),
    );

    _scanSub = FlutterBluePlus.onScanResults.listen((results) async {
      if (results.isEmpty) return;
      await FlutterBluePlus.stopScan();
      await _connect(results.last.device);
    });
  }

  static Future<void> _connect(BluetoothDevice device) async {
    _device = device;
    await device.connect(license: License.free);
    print('[BLE] Connected to ${device.platformName}');

    final services = await device.discoverServices();
    print('[BLE] Found ${services.length} services');
    for (final service in services) {
      print('[BLE] Service: ${service.serviceUuid}');
      if (service.serviceUuid == Guid(_serviceUuid)) {
        for (final char in service.characteristics) {
          print('[BLE] Characteristic: ${char.characteristicUuid}');
          if (char.characteristicUuid == Guid(_charUuid)) {
            await char.setNotifyValue(true);
            print('[BLE] Subscribed to notifications');
            _notifySub = char.onValueReceived.listen((bytes) {
              final raw = String.fromCharCodes(bytes);
              print('[BLE] Raw payload: $raw');
              final payload = PiPayload.fromString(raw);
              _payloadController.add(payload);
            });
          }
        }
      }
    }
  }

  static void stop() {
    _scanSub?.cancel();
    _notifySub?.cancel();
    _device?.disconnect();
  }
}