import 'package:flutter/material.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:latlong2/latlong.dart';
import '../models/models.dart';
import '../theme/app_theme.dart';

class RouteMapWidget extends StatelessWidget {
  final List<GpsPoint> points;

  const RouteMapWidget({super.key, required this.points});

  @override
  Widget build(BuildContext context) {
    if (points.isEmpty) {
      return Container(
        height: 320,
        decoration: BoxDecoration(
          color: AppTheme.surface,
          borderRadius: BorderRadius.circular(12),
          border: Border.all(color: AppTheme.border),
        ),
        child: const Center(child: Text('No GPS data', style: TextStyle(color: AppTheme.textSecondary))),
      );
    }

    final latLngs = points.map((p) => LatLng(p.lat, p.lng)).toList();
    final center = LatLng(
      points.map((p) => p.lat).reduce((a, b) => a + b) / points.length,
      points.map((p) => p.lng).reduce((a, b) => a + b) / points.length,
    );

    return Container(
      decoration: BoxDecoration(
        color: AppTheme.surface,
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: AppTheme.border),
      ),
      child: ClipRRect(
        borderRadius: BorderRadius.circular(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
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
                Text('${points.length} GPS points', style: const TextStyle(
                  fontSize: 11, color: AppTheme.textMuted,
                )),
              ]),
            ),
            SizedBox(
              height: 320,
              child: FlutterMap(
                options: MapOptions(
                  initialCenter: center,
                  initialZoom: 15.0,
                ),
                children: [
                  TileLayer(
                    urlTemplate: 'https://tile.openstreetmap.org/{z}/{x}/{y}.png',
                    userAgentPackageName: 'com.example.pi_tracker',
                  ),
                  // Route polyline
                  PolylineLayer(
                    polylines: [
                      Polyline(
                        points: latLngs,
                        strokeWidth: 3.5,
                        color: AppTheme.accentBlue,
                      ),
                    ],
                  ),
                  // Start marker
                  MarkerLayer(markers: [
                    Marker(
                      point: latLngs.first,
                      width: 16,
                      height: 16,
                      child: Container(
                        decoration: BoxDecoration(
                          color: AppTheme.accentGreen,
                          shape: BoxShape.circle,
                          border: Border.all(color: Colors.white, width: 2),
                        ),
                      ),
                    ),
                    // End / current marker
                    Marker(
                      point: latLngs.last,
                      width: 16,
                      height: 16,
                      child: Container(
                        decoration: BoxDecoration(
                          color: AppTheme.accent,
                          shape: BoxShape.circle,
                          border: Border.all(color: Colors.white, width: 2),
                        ),
                      ),
                    ),
                  ]),
                ],
              ),
            ),
            Padding(
              padding: const EdgeInsets.fromLTRB(20, 12, 20, 16),
              child: Row(children: [
                _MapLegend(color: AppTheme.accentGreen, label: 'Start'),
                const SizedBox(width: 20),
                _MapLegend(color: AppTheme.accent, label: 'Current'),
                const SizedBox(width: 20),
                _MapLegend(color: AppTheme.accentBlue, label: 'Route', isLine: true),
              ]),
            ),
          ],
        ),
      ),
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
          : Container(
              width: 10, height: 10,
              decoration: BoxDecoration(color: color, shape: BoxShape.circle),
            ),
      const SizedBox(width: 6),
      Text(label, style: const TextStyle(fontSize: 11, color: AppTheme.textSecondary)),
    ]);
  }
}