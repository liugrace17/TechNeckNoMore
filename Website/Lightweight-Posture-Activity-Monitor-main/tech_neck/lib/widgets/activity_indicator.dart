import 'package:flutter/material.dart';
import '../theme/app_theme.dart';

class ActivityIndicator extends StatelessWidget {
  final String currentActivity;

  const ActivityIndicator({super.key, required this.currentActivity});

  @override
  Widget build(BuildContext context) {
    final activity = currentActivity.toLowerCase();
    final Color color;
    final String label;
    final IconData icon;

    switch (activity) {
      case 'active':
        color = AppTheme.accentGreen;
        label = 'ACTIVE';
        icon = Icons.directions_walk_rounded;
      case 'idle':
        color = AppTheme.textSecondary;
        label = 'IDLE';
        icon = Icons.airline_seat_recline_extra;
      case 'standing':
        color = AppTheme.accentBlue;
        label = 'STANDING';
        icon = Icons.person_rounded; 
      default:
        color = AppTheme.textMuted;
        label = activity.toUpperCase();
        icon = Icons.help_outline_rounded;
    }

    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 10),
      decoration: const BoxDecoration(
        color: AppTheme.surface,
        border: Border(bottom: BorderSide(color: AppTheme.border)),
      ),
      child: Row(
        children: [
          Container(
            width: 8,
            height: 8,
            decoration: BoxDecoration(color: color, shape: BoxShape.circle),
          ),
          const SizedBox(width: 10),
          const Text('LIVE', style: TextStyle(
            fontSize: 10, letterSpacing: 2,
            color: AppTheme.textMuted, fontWeight: FontWeight.w700,
          )),
          const SizedBox(width: 16),
          Icon(icon, color: color, size: 14),
          const SizedBox(width: 6),
          Text(label, style: TextStyle(
            fontSize: 11, letterSpacing: 1.5,
            color: color, fontWeight: FontWeight.w700,
          )),
        ],
      ),
    );
  }
}
