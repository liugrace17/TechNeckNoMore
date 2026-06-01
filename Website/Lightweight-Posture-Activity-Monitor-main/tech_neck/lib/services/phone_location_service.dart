import 'dart:async';
import 'package:geolocator/geolocator.dart';
import 'package:latlong2/latlong.dart';

class PhoneLocationService {
  static final _controller = StreamController<LatLng>.broadcast();
  static Stream<LatLng> get stream => _controller.stream;

  static StreamSubscription<Position>? _positionSub;

  static Future<void> start() async {
    bool serviceEnabled = await Geolocator.isLocationServiceEnabled();
    if (!serviceEnabled) return;

    LocationPermission permission = await Geolocator.requestPermission();
    if (permission == LocationPermission.denied) {
      permission = await Geolocator.requestPermission();
      if (permission == LocationPermission.denied) return;
    }
    if (permission == LocationPermission.deniedForever) return;

    _positionSub = Geolocator.getPositionStream(
      locationSettings: const LocationSettings(
        accuracy: LocationAccuracy.high,
        distanceFilter: 5,
      ),
    ).listen((position) {
      _controller.add(LatLng(position.latitude, position.longitude));
    });
  }

  static void stop() {
    _positionSub?.cancel();
  }
}