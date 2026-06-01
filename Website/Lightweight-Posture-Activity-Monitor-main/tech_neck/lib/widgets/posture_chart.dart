import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';
import '../theme/app_theme.dart';

class PostureDonutChart extends StatelessWidget {
  final double percentage;
  final int activeMinutes;
  final int walkingGoalMinutes;
  final int steps;
  final int goalSteps;

  const PostureDonutChart({
    super.key,
    required this.percentage,
    required this.activeMinutes,
    this.walkingGoalMinutes = 5,
    required this.steps,
    this.goalSteps = 100,
  });

  @override
  Widget build(BuildContext context) {
    final good = percentage.clamp(0.0, 100.0);
    final poor = 100.0 - good;
    final postureColor = good >= 75
        ? AppTheme.accentGreen
        : good >= 50
            ? AppTheme.accent
            : AppTheme.accentRed;

    final walkProgress = (activeMinutes / walkingGoalMinutes).clamp(0.0, 1.0);
    final walkColor = walkProgress >= 1.0
        ? AppTheme.accentGreen
        : walkProgress >= 0.5
            ? AppTheme.accent
            : AppTheme.accentRed;

    final stepProgress = (steps / goalSteps).clamp(0.0, 1.0);
    final stepColor = stepProgress >= 1.0
        ? AppTheme.accentGreen
        : stepProgress >= 0.5
            ? AppTheme.accent
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
          // Posture header
          Row(children: [
            Icon(Icons.accessibility_new_rounded, color: postureColor, size: 16),
            const SizedBox(width: 8),
            const Text('POSTURE QUALITY', style: TextStyle(
              fontSize: 11, letterSpacing: 1.8,
              color: AppTheme.textSecondary, fontWeight: FontWeight.w700,
            )),
          ]),
          const SizedBox(height: 16),
          // Donut chart + legend
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
                            color: postureColor,
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
                        fontSize: 20,
                        fontWeight: FontWeight.w700,
                        color: postureColor,
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
                    _Legend(color: postureColor, label: 'At goal posture', value: '${good.toStringAsFixed(0)}%'),
                    const SizedBox(height: 10),
                    _Legend(color: AppTheme.border, label: 'Poor posture', value: '${poor.toStringAsFixed(0)}%'),
                    const SizedBox(height: 16),
                    Text(
                      good >= 75 ? '✓ On track' : good >= 50 ? '~ Close to goal' : '✗ Needs attention',
                      style: TextStyle(fontSize: 12, color: postureColor, fontWeight: FontWeight.w700),
                    ),
                    const Text('Goal: 75%', style: TextStyle(fontSize: 11, color: AppTheme.textMuted)),
                  ],
                ),
              ),
            ],
          ),
          const SizedBox(height: 16),
          const Divider(color: AppTheme.border, height: 1),
          const SizedBox(height: 16),
          // Active minutes goal
          Row(children: [
            Icon(Icons.directions_run_rounded, color: walkColor, size: 16),
            const SizedBox(width: 8),
            const Text('ACTIVE GOAL', style: TextStyle(
              fontSize: 11, letterSpacing: 1.8,
              color: AppTheme.textSecondary, fontWeight: FontWeight.w700,
            )),
            const Spacer(),
            Text(
              '$activeMinutes / ${walkingGoalMinutes}min',
              style: TextStyle(fontSize: 11, color: walkColor, fontWeight: FontWeight.w700),
            ),
          ]),
          const SizedBox(height: 10),
          ClipRRect(
            borderRadius: BorderRadius.circular(2),
            child: LinearProgressIndicator(
              value: walkProgress,
              backgroundColor: AppTheme.border,
              valueColor: AlwaysStoppedAnimation<Color>(walkColor),
              minHeight: 6,
            ),
          ),
          const SizedBox(height: 6),
          Text(
            walkProgress >= 1.0 ? '✓ Daily goal met' : '${((1 - walkProgress) * walkingGoalMinutes).round()} min remaining',
            style: TextStyle(fontSize: 11, color: walkProgress >= 1.0 ? AppTheme.accentGreen : AppTheme.textMuted),
          ),
          const SizedBox(height: 16),
          const Divider(color: AppTheme.border, height: 1),
          const SizedBox(height: 16),
          // Step count goal
          Row(children: [
            Icon(Icons.directions_walk_rounded, color: stepColor, size: 16),
            const SizedBox(width: 8),
            const Text('STEP GOAL', style: TextStyle(
              fontSize: 11, letterSpacing: 1.8,
              color: AppTheme.textSecondary, fontWeight: FontWeight.w700,
            )),
            const Spacer(),
            Text(
              '$steps steps / $goalSteps completed',
              style: TextStyle(fontSize: 7, color: stepColor, fontWeight: FontWeight.w700),
            ),
          ]),
          const SizedBox(height: 10),
          ClipRRect(
            borderRadius: BorderRadius.circular(2),
            child: LinearProgressIndicator(
              value: stepProgress,
              backgroundColor: AppTheme.border,
              valueColor: AlwaysStoppedAnimation<Color>(stepColor),
              minHeight: 6,
            ),
          ),
          const SizedBox(height: 6),
          Text(
            stepProgress >= 1.0 ? '✓ Daily goal met' : '${goalSteps - steps} steps remaining',
            style: TextStyle(fontSize: 11, color: stepProgress >= 1.0 ? AppTheme.accentGreen : AppTheme.textMuted),
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
