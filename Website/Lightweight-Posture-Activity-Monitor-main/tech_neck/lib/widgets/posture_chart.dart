import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';
import '../theme/app_theme.dart';

class PostureDonutChart extends StatelessWidget {
  final double percentage; // 0–100, % of time at goal posture

  const PostureDonutChart({super.key, required this.percentage});

  @override
  Widget build(BuildContext context) {
    final good = percentage.clamp(0.0, 100.0);
    final poor = 100.0 - good;
    final color = good >= 75 ? AppTheme.accentGreen
        : good >= 50 ? AppTheme.accent
        : AppTheme.accentRed;

    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: AppTheme.surface,
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: AppTheme.border),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(children: [
            Icon(Icons.accessibility_new_rounded, color: color, size: 16),
            const SizedBox(width: 8),
            const Text('POSTURE QUALITY', style: TextStyle(
              fontSize: 11, letterSpacing: 1.8,
              color: AppTheme.textSecondary, fontWeight: FontWeight.w700,
            )),
          ]),
          const SizedBox(height: 16),
          Row(
            children: [
              SizedBox(
                height: 120,
                width: 120,
                child: Stack(
                  alignment: Alignment.center,
                  children: [
                    PieChart(
                      PieChartData(
                        startDegreeOffset: -90,
                        sectionsSpace: 2,
                        centerSpaceRadius: 38,
                        sections: [
                          PieChartSectionData(
                            value: good,
                            color: color,
                            radius: 20,
                            showTitle: false,
                          ),
                          PieChartSectionData(
                            value: poor,
                            color: AppTheme.border,
                            radius: 20,
                            showTitle: false,
                          ),
                        ],
                      ),
                    ),
                    Text(
                      '${good.toStringAsFixed(0)}%',
                      style: TextStyle(
                        fontSize: 20, fontWeight: FontWeight.w700,
                        color: color,
                      ),
                    ),
                  ],
                ),
              ),
              const SizedBox(width: 20),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    _Legend(color: color, label: 'At goal posture', value: '${good.toStringAsFixed(0)}%'),
                    const SizedBox(height: 10),
                    _Legend(color: AppTheme.border, label: 'Poor posture', value: '${poor.toStringAsFixed(0)}%'),
                    const SizedBox(height: 16),
                    Text(
                      good >= 75 ? '✓ On track' : good >= 50 ? '~ Close to goal' : '✗ Needs attention',
                      style: TextStyle(fontSize: 12, color: color, fontWeight: FontWeight.w700),
                    ),
                    Text('Goal: 75%', style: const TextStyle(fontSize: 11, color: AppTheme.textMuted)),
                  ],
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }
}

class _Legend extends StatelessWidget {
  final Color color;
  final String label;
  final String value;

  const _Legend({required this.color, required this.label, required this.value});

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        Container(width: 10, height: 10, decoration: BoxDecoration(
          color: color, borderRadius: BorderRadius.circular(2),
        )),
        const SizedBox(width: 8),
        Expanded(child: Text(label, style: const TextStyle(fontSize: 11, color: AppTheme.textSecondary))),
        Text(value, style: const TextStyle(fontSize: 11, color: AppTheme.textPrimary, fontWeight: FontWeight.w700)),
      ],
    );
  }
}