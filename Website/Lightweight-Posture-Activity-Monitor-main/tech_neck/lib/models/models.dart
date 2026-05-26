class ActivitySummary {
  final int steps;
  final int activeMinutes;
  final double postureGoalPercentage;
  final String currentActivity;
  final int idleStreakMinutes;

  ActivitySummary({
    required this.steps,
    required this.activeMinutes,
    required this.postureGoalPercentage,
    required this.currentActivity,
    required this.idleStreakMinutes,
  });

  factory ActivitySummary.fromJson(Map<String, dynamic> json) => ActivitySummary(
        steps: json['steps'],
        activeMinutes: json['active_minutes'],
        postureGoalPercentage: (json['posture_goal_percentage'] as num).toDouble(),
        currentActivity: json['current_activity'],
        idleStreakMinutes: json['idle_streak_minutes'],
      );
}

class GpsPoint {
  final double lat;
  final double lng;

  GpsPoint({required this.lat, required this.lng});

  factory GpsPoint.fromJson(Map<String, dynamic> json) => GpsPoint(
        lat: (json['lat'] as num).toDouble(),
        lng: (json['lng'] as num).toDouble(),
      );
}