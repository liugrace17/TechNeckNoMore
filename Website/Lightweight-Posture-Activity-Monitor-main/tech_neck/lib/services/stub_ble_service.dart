import 'dart:async';
import 'ble_service.dart' show PiPayload;

class BleService {
  static final _payloadController = StreamController<PiPayload>.broadcast();
  static Stream<PiPayload> get stream => _payloadController.stream;

  static Future<void> start() async {}
  static void stop() {}
}