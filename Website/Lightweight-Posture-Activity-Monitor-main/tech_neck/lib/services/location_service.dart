import 'dart:async';
import 'dart:io' show Platform;
import 'package:flutter/foundation.dart' show kIsWeb;
import 'package:latlong2/latlong.dart';

import 'ble_service.dart'
    if (dart.library.html) 'stub_ble_service.dart';
import 'phone_location_service.dart'
    if (dart.library.html) 'stub_phone_location_service.dart';

enum GpsSource { bluetooth, phone, http }

class LocationPoint {
  final LatLng position;
  final GpsSource source;
  LocationPoint(this.position, this.source);
}

class LocationService {
  final _locationController = StreamController<LocationPoint>.broadcast();
  Stream<LocationPoint> get locationStream => _locationController.stream;

  bool _piHasGpsFix = false;
  bool _bleConnected = false;
  StreamSubscription? _bleSub;
  StreamSubscription? _phoneSub;
  Timer? _bleConnectionTimer;

  Future<void> start() async {
    if (kIsWeb) return;
    if (!Platform.isAndroid && !Platform.isIOS) return;

    _bleConnectionTimer = Timer(const Duration(seconds: 15), () {
      if (!_bleConnected) _startPhoneGps();
    });

    _bleSub = BleService.stream.listen((payload) {
      _bleConnected = true;
      _bleConnectionTimer?.cancel();

      if (payload.hasGps) {
        _piHasGpsFix = true;
        _stopPhoneGps();
        _locationController.add(LocationPoint(payload.position!, GpsSource.bluetooth));
      } else if (!_piHasGpsFix) {
        _startPhoneGps();
      }
    });

    await BleService.start();
  }

  void _startPhoneGps() {
    if (_phoneSub != null) return;
    _phoneSub = PhoneLocationService.stream.listen((latLng) {
      if (!_piHasGpsFix) {
        _locationController.add(LocationPoint(latLng, GpsSource.phone));
      }
    });
    PhoneLocationService.start();
  }

  void _stopPhoneGps() {
    _phoneSub?.cancel();
    _phoneSub = null;
    PhoneLocationService.stop();
  }

  void dispose() {
    _bleConnectionTimer?.cancel();
    _bleSub?.cancel();
    _stopPhoneGps();
    BleService.stop();
    _locationController.close();
  }
}