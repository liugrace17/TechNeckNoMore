import 'dart:async';
import 'package:latlong2/latlong.dart';

class PhoneLocationService {
  static final _controller = StreamController<LatLng>.broadcast();
  static Stream<LatLng> get stream => _controller.stream;

  static Future<void> start() async {}
  static void stop() {}
}