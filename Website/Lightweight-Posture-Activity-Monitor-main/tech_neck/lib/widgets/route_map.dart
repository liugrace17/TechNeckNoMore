import 'package:flutter/foundation.dart' show kIsWeb;
import 'package:flutter/material.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:latlong2/latlong.dart';
import '../models/models.dart';
import '../services/location_service.dart';
import '../theme/app_theme.dart';

class RouteMapWidget extends StatelessWidget {
  final List<GpsPoint> httpPoints;
  final List<LocationPoint> livePoints;

  const RouteMapWidget({
    super.key,
    this.httpPoints = const [],
    this.livePoints = const [],
  });

  @override
  Widget build(BuildContext context) {
    final List<LatLng> latLngs = kIsWeb
        ? httpPoints.map((p) => LatLng(p.lat, p.lng)).toList()
        : livePoints.map((p) => p.position).toList();

    final GpsSource? lastSource = (!kIsWeb && livePoints.isNotEmpty)
        ? livePoints.last.source
        : null;

    if (latLngs.isEmpty) {
      return _shell(
        lastSource: null,
        latLngs: [],
        child: const Center(
          child: Text('Waiting for GPS data...', style: TextStyle(color: AppTheme.textSecondary)),
        ),
      );
    }

    final center = LatLng(
      latLngs.map((p) => p.latitude).reduce((a, b) => a + b) / latLngs.length,
      latLngs.map((p) => p.longitude).reduce((a, b) => a + b) / latLngs.length,
    );

    return _shell(
      lastSource: lastSource,
      latLngs: latLngs,
      child: FlutterMap(
        options: MapOptions(initialCenter: center, initialZoom: 15.0),
        children: [
          TileLayer(
            urlTemplate: 'https://tile.openstreetmap.org/{z}/{x}/{y}.png',
            userAgentPackageName: 'com.example.pi_tracker',
          ),
          PolylineLayer(polylines: [
            Polyline(points: latLngs, strokeWidth: 3.5, color: AppTheme.accentBlue),
          ]),
          MarkerLayer(markers: [
            Marker(
              point: latLngs.first, width: 16, height: 16,
              child: Container(decoration: BoxDecoration(
                color: AppTheme.accentGreen, shape: BoxShape.circle,
                border: Border.all(color: Colors.white, width: 2),
              )),
            ),
            Marker(
              point: latLngs.last, width: 16, height: 16,
              child: Container(decoration: BoxDecoration(
                color: AppTheme.accent, shape: BoxShape.circle,
                border: Border.all(color: Colors.white, width: 2),
              )),
            ),
          ]),
        ],
      ),
    );
  }

  Widget _shell({required Widget child, required GpsSource? lastSource, required List<LatLng> latLngs}) {
    return Container(
      decoration: BoxDecoration(
        color: AppTheme.surface,
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: AppTheme.border),
      ),
      child: ClipRRect(
        borderRadius: BorderRadius.circular(12),
        child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
          Padding(
            padding: const EdgeInsets.fromLTRB(20, 16, 20, 12),
            child: Row(children: [
              const Icon(Icons.map_outlined, color: AppTheme.accentBlue, size: 16),
              const SizedBox(width: 8),
              const Text("TODAY'S ROUTE", style: TextStyle(
                fontSize: 11, letterSpacing: 1.8,
                color: AppTheme.textSecondary, fontWeight: FontWeight.w700,
              )),
              const Spacer(),
              if (lastSource != null) _SourceBadge(lastSource),
              if (latLngs.isNotEmpty) ...[
                const SizedBox(width: 12),
                Text('${latLngs.length} pts', style: const TextStyle(fontSize: 11, color: AppTheme.textMuted)),
              ],
            ]),
          ),
          SizedBox(height: 320, child: child),
          if (latLngs.isNotEmpty) Padding(
            padding: const EdgeInsets.fromLTRB(20, 12, 20, 16),
            child: Row(children: [
              _MapLegend(color: AppTheme.accentGreen, label: 'Start'),
              const SizedBox(width: 20),
              _MapLegend(color: AppTheme.accent, label: 'Current'),
              const SizedBox(width: 20),
              _MapLegend(color: AppTheme.accentBlue, label: 'Route', isLine: true),
            ]),
          ),
        ]),
      ),
    );
  }
}

class _SourceBadge extends StatelessWidget {
  final GpsSource source;
  const _SourceBadge(this.source);

  @override
  Widget build(BuildContext context) {
    final (label, color) = switch (source) {
      GpsSource.bluetooth => ('BLE', AppTheme.accentBlue),
      GpsSource.phone     => ('Phone GPS', AppTheme.accent),
      GpsSource.http      => ('HTTP', AppTheme.accentGreen),
    };
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 3),
      decoration: BoxDecoration(
        color: color.withOpacity(0.15),
        borderRadius: BorderRadius.circular(4),
        border: Border.all(color: color.withOpacity(0.4)),
      ),
      child: Text(label, style: TextStyle(fontSize: 10, color: color, fontWeight: FontWeight.w700)),
    );
  }
}

class _MapLegend extends StatelessWidget {
  final Color color;
  final String label;
  final bool isLine;
  const _MapLegend({required this.color, required this.label, this.isLine = false});

  @override
  Widget build(BuildContext context) {
    return Row(children: [
      isLine
          ? Container(width: 16, height: 3, color: color)
          : Container(width: 10, height: 10, decoration: BoxDecoration(color: color, shape: BoxShape.circle)),
      const SizedBox(width: 6),
      Text(label, style: const TextStyle(fontSize: 11, color: AppTheme.textSecondary)),
    ]);
  }
}