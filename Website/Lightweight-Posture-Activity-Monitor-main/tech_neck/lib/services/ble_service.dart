import 'dart:async';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:latlong2/latlong.dart';

class PiPayload {
  final int steps;
  final int activeMinutes;
  final double postureGoalPercentage;
  final double? lat;
  final double? lng;

  PiPayload({
    required this.steps,
    required this.activeMinutes,
    required this.postureGoalPercentage,
    this.lat,
    this.lng,
  });

  bool get hasGps => lat != null && lng != null;
  LatLng? get position => hasGps ? LatLng(lat!, lng!) : null;

  // Format: "steps,active_minutes,posture_pct,lat,lng"
  factory PiPayload.fromString(String raw) {
    final p = raw.trim().split(',');
    if (p.length != 5) return PiPayload(steps: 0, activeMinutes: 0, postureGoalPercentage: 0);
    return PiPayload(
      steps:                 int.tryParse(p[0]) ?? 0,
      activeMinutes:         int.tryParse(p[1]) ?? 0,
      postureGoalPercentage: double.tryParse(p[2]) ?? 0,
      lat:                   double.tryParse(p[3]),
      lng:                   double.tryParse(p[4]),
    );
  }
}

class BleService {
  static const String _serviceUuid = "66bffa4d-fdb1-4a44-9fcb-b19fa257b833";
  static const String _charUuid    = "dbbcc4ab-0707-442b-a572-fbfdc5e9ebed";

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

    final services = await device.discoverServices();
    for (final service in services) {
      if (service.serviceUuid == Guid(_serviceUuid)) {
        for (final char in service.characteristics) {
          if (char.characteristicUuid == Guid(_charUuid)) {
            await char.setNotifyValue(true);
            _notifySub = char.lastValueStream.listen((bytes) {
              final payload = PiPayload.fromString(String.fromCharCodes(bytes));
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