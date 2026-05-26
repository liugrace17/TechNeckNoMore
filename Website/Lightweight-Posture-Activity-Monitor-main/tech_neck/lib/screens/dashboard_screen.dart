import 'dart:async';
import 'package:flutter/material.dart';
import '../models/models.dart';
import '../services/api_service.dart';
import '../theme/app_theme.dart';
import '../widgets/stat_card.dart';
import '../widgets/posture_chart.dart';
import '../widgets/route_map.dart';
import '../widgets/activity_indicator.dart';

class DashboardScreen extends StatefulWidget {
  const DashboardScreen({super.key});

  @override
  State<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen> {
  ActivitySummary? _summary;
  List<GpsPoint> _route = [];
  bool _loading = true;
  String? _error;
  Timer? _pollTimer;

  @override
  void initState() {
    super.initState();
    _loadAll();
    _pollTimer = Timer.periodic(const Duration(seconds: 3), (_) => _silentRefresh());
  }

  @override
  void dispose() {
    _pollTimer?.cancel();
    super.dispose();
  }

  Future<void> _silentRefresh() async {
    try {
      final summary = await ApiService.getActivitySummary();
      if (mounted) {
        setState(() { _summary = summary; _error = null; });
      }
    } catch (_) {}
  }

  Future<void> _loadAll() async {
    setState(() { _loading = true; _error = null; });
    try {
      final results = await Future.wait([
        ApiService.getActivitySummary(),
        ApiService.getGpsRoute(),
      ]);
      setState(() {
        _summary = results[0] as ActivitySummary;
        _route = results[1] as List<GpsPoint>;
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
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: AppTheme.bg,
      body: Column(
        children: [
          _buildHeader(),
          if (!_loading && _error == null && _summary != null)
            ActivityIndicator(currentActivity: _summary!.currentActivity),
          Expanded(
            child: _loading
                ? const Center(child: CircularProgressIndicator(color: AppTheme.accent))
                : _error != null
                    ? _buildError()
                    : _buildDashboard(),
          ),
        ],
      ),
    );
  }

  Widget _buildHeader() {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 16),
      decoration: const BoxDecoration(
        color: AppTheme.surface,
        border: Border(bottom: BorderSide(color: AppTheme.border)),
      ),
      child: Row(
        children: [
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
          IconButton(
            icon: const Icon(Icons.refresh_rounded, size: 18),
            color: AppTheme.textSecondary,
            tooltip: 'Refresh',
            onPressed: _loadAll,
          ),
        ],
      ),
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
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Icon(Icons.wifi_off_rounded, color: AppTheme.accentRed, size: 40),
            const SizedBox(height: 16),
            Text(_error ?? '', textAlign: TextAlign.center,
              style: const TextStyle(fontSize: 13, color: AppTheme.textSecondary, height: 1.6)),
            const SizedBox(height: 24),
            ElevatedButton.icon(
              onPressed: _loadAll,
              icon: const Icon(Icons.refresh_rounded, size: 16),
              label: const Text('Retry'),
              style: ElevatedButton.styleFrom(
                backgroundColor: AppTheme.accent,
                foregroundColor: Colors.black,
                padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildDashboard() {
    final s = _summary!;
    final isWide = MediaQuery.of(context).size.width > 800;

    return SingleChildScrollView(
      padding: const EdgeInsets.all(24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          isWide
              ? Row(children: [
                  Expanded(child: StatCard(
                    label: 'Steps',
                    value: s.steps.toString(),
                    icon: Icons.directions_walk_rounded,
                    accentColor: AppTheme.accent,
                  )),
                  const SizedBox(width: 16),
                  Expanded(child: StatCard(
                    label: 'Active Time',
                    value: '${s.activeMinutes ~/ 60}h ${s.activeMinutes % 60}m',
                    icon: Icons.directions_run_rounded,
                    accentColor: AppTheme.accentGreen,
                    child: _IdleStreak(minutes: s.idleStreakMinutes),
                  )),
                  const SizedBox(width: 16),
                  Expanded(child: PostureDonutChart(
                    percentage: s.postureGoalPercentage,
                    activeMinutes: s.activeMinutes,
                  )),
                ])
              : Column(children: [
                  StatCard(
                    label: 'Steps',
                    value: s.steps.toString(),
                    icon: Icons.directions_walk_rounded,
                    accentColor: AppTheme.accent,
                  ),
                  const SizedBox(height: 16),
                  StatCard(
                    label: 'Active Time',
                    value: '${s.activeMinutes ~/ 60}h ${s.activeMinutes % 60}m',
                    icon: Icons.directions_run_rounded,
                    accentColor: AppTheme.accentGreen,
                    child: _IdleStreak(minutes: s.idleStreakMinutes),
                  ),
                  const SizedBox(height: 16),
                  PostureDonutChart(
                    percentage: s.postureGoalPercentage,
                    activeMinutes: s.activeMinutes,
                  ),
                ]),
          const SizedBox(height: 24),
          RouteMapWidget(points: _route),
          const SizedBox(height: 32),
        ],
      ),
    );
  }
}

class _IdleStreak extends StatelessWidget {
  final int minutes;

  const _IdleStreak({required this.minutes});

  @override
  Widget build(BuildContext context) {
    final Color color;
    final String label;

    if (minutes >= 15) {
      color = AppTheme.accentRed;
      label = 'Long idle streak — time to move';
    } else if (minutes >= 5) {
      color = AppTheme.accent;
      label = 'Idle streak building up';
    } else {
      color = AppTheme.accentGreen;
      label = 'Idle streak looks good';
    }

    return Row(
      children: [
        Icon(Icons.timer_outlined, size: 12, color: color),
        const SizedBox(width: 6),
        Text(
          '$minutes min idle streak  ·  $label',
          style: TextStyle(fontSize: 11, color: color),
        ),
      ],
    );
  }
}
