import 'dart:async';
import 'package:flutter/foundation.dart' show kIsWeb;
import 'package:flutter/material.dart';
import '../models/models.dart';
import '../services/api_service.dart';
import '../services/ble_service.dart'
    if (dart.library.html) '../services/stub_ble_service.dart';
import '../services/location_service.dart';
import '../theme/app_theme.dart';
import '../widgets/stat_card.dart';
import '../widgets/posture_chart.dart';
import '../widgets/route_map.dart';

class DashboardScreen extends StatefulWidget {
  const DashboardScreen({super.key});

  @override
  State<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen> {
  ActivitySummary? _summary;
  List<GpsPoint> _httpRoute = [];
  bool _loading = true;
  String? _error;

  final LocationService _locationService = LocationService();
  final List<LocationPoint> _livePoints = [];
  StreamSubscription? _bleSub;

  @override
  void initState() {
    super.initState();
    if (kIsWeb) {
      _loadWebData();
    } else {
      _initMobile();
    }
  }

  Future<void> _initMobile() async {
    setState(() {
      _summary = ActivitySummary(steps: 0, standingMinutes: 0, postureGoalPercentage: 0);
      _loading = false;
    });

    _bleSub = BleService.stream.listen((payload) {
      setState(() {
        _summary = ActivitySummary(
          steps: payload.steps,
          standingMinutes: payload.activeMinutes,
          postureGoalPercentage: payload.postureGoalPercentage,
        );
      });
    });

    _locationService.locationStream.listen((point) {
      setState(() => _livePoints.add(point));
    });

    await _locationService.start();
  }

  Future<void> _loadWebData() async {
    setState(() { _loading = true; _error = null; });
    try {
      final results = await Future.wait([
        ApiService.getActivitySummary(),
        ApiService.getGpsRoute(),
      ]);
      setState(() {
        _summary = results[0] as ActivitySummary;
        _httpRoute = results[1] as List<GpsPoint>;
        _loading = false;
      });
    } catch (e) {
      setState(() {
        _error = 'Could not connect to Pi server.\nMake sure pi_server.py is running on localhost:8000.';
        _loading = false;
      });
    }
  }

  @override
  void dispose() {
    _bleSub?.cancel();
    _locationService.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: AppTheme.bg,
      body: Column(children: [
        _buildHeader(),
        Expanded(
          child: _loading
              ? const Center(child: CircularProgressIndicator(color: AppTheme.accent))
              : _error != null
                  ? _buildError()
                  : _summary == null
                      ? const Center(child: Text('Waiting for Pi...', style: TextStyle(color: AppTheme.textSecondary)))
                      : _buildDashboard(),
        ),
      ]),
    );
  }

  Widget _buildHeader() {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 16),
      decoration: const BoxDecoration(
        color: AppTheme.surface,
        border: Border(bottom: BorderSide(color: AppTheme.border)),
      ),
      child: Row(children: [
        Container(
          width: 28, height: 28,
          decoration: BoxDecoration(color: AppTheme.accent, borderRadius: BorderRadius.circular(6)),
          child: const Icon(Icons.show_chart, color: Colors.black, size: 16),
        ),
        const SizedBox(width: 12),
        const Text('PI TRACKER', style: TextStyle(
          fontSize: 15, fontWeight: FontWeight.w700,
          color: AppTheme.textPrimary, letterSpacing: 2,
        )),
        const Spacer(),
        if (!kIsWeb && _livePoints.isNotEmpty)
          _GpsSourceIndicator(_livePoints.last.source),
        if (kIsWeb)
          IconButton(
            icon: const Icon(Icons.refresh_rounded, size: 18),
            color: AppTheme.textSecondary,
            tooltip: 'Refresh',
            onPressed: _loadWebData,
          ),
      ]),
    );
  }

  Widget _buildError() {
    return Center(
      child: Container(
        constraints: const BoxConstraints(maxWidth: 420),
        padding: const EdgeInsets.all(32),
        margin: const EdgeInsets.all(24),
        decoration: BoxDecoration(
          color: AppTheme.surface,
          borderRadius: BorderRadius.circular(12),
          border: Border.all(color: AppTheme.accentRed.withOpacity(0.4)),
        ),
        child: Column(mainAxisSize: MainAxisSize.min, children: [
          const Icon(Icons.wifi_off_rounded, color: AppTheme.accentRed, size: 40),
          const SizedBox(height: 16),
          Text(_error ?? '', textAlign: TextAlign.center,
            style: const TextStyle(fontSize: 13, color: AppTheme.textSecondary, height: 1.6)),
          const SizedBox(height: 24),
          ElevatedButton.icon(
            onPressed: _loadWebData,
            icon: const Icon(Icons.refresh_rounded, size: 16),
            label: const Text('Retry'),
            style: ElevatedButton.styleFrom(
              backgroundColor: AppTheme.accent,
              foregroundColor: Colors.black,
              padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
            ),
          ),
        ]),
      ),
    );
  }

  Widget _buildDashboard() {
    final s = _summary!;
    final isWide = MediaQuery.of(context).size.width > 800;

    return SingleChildScrollView(
      padding: const EdgeInsets.all(24),
      child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
        isWide
            ? Row(children: [
                Expanded(child: StatCard(
                  label: 'Steps', value: s.steps.toString(),
                  icon: Icons.directions_walk_rounded, accentColor: AppTheme.accent,
                )),
                const SizedBox(width: 16),
                Expanded(child: StatCard(
                  label: 'Active',
                  value: '${s.standingMinutes ~/ 60}h ${s.standingMinutes % 60}m',
                  icon: Icons.airline_seat_recline_extra_rounded, accentColor: AppTheme.accentGreen,
                )),
                const SizedBox(width: 16),
                Expanded(child: PostureDonutChart(percentage: s.postureGoalPercentage)),
              ])
            : Column(children: [
                StatCard(
                  label: 'Steps', value: s.steps.toString(),
                  icon: Icons.directions_walk_rounded, accentColor: AppTheme.accent,
                ),
                const SizedBox(height: 16),
                StatCard(
                  label: 'Active',
                  value: '${s.standingMinutes ~/ 60}h ${s.standingMinutes % 60}m',
                  icon: Icons.airline_seat_recline_extra_rounded, accentColor: AppTheme.accentGreen,
                ),
                const SizedBox(height: 16),
                PostureDonutChart(percentage: s.postureGoalPercentage),
              ]),
        const SizedBox(height: 24),
        RouteMapWidget(httpPoints: _httpRoute, livePoints: _livePoints),
        const SizedBox(height: 32),
      ]),
    );
  }
}

class _GpsSourceIndicator extends StatelessWidget {
  final GpsSource source;
  const _GpsSourceIndicator(this.source);

  @override
  Widget build(BuildContext context) {
    final (label, color, icon) = switch (source) {
      GpsSource.bluetooth => ('BLE', AppTheme.accentBlue, Icons.bluetooth),
      GpsSource.phone     => ('Phone GPS', AppTheme.accent, Icons.gps_fixed),
      GpsSource.http      => ('HTTP', AppTheme.accentGreen, Icons.wifi),
    };
    return Row(children: [
      Icon(icon, color: color, size: 14),
      const SizedBox(width: 4),
      Text(label, style: TextStyle(fontSize: 11, color: color)),
      const SizedBox(width: 12),
    ]);
  }
}