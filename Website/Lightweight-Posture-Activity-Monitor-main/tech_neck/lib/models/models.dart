class ActivitySummary {
  final int steps;
  final int standingMinutes;
  final double postureGoalPercentage;

  ActivitySummary({
    required this.steps,
    required this.standingMinutes,
    required this.postureGoalPercentage,
  });

  factory ActivitySummary.fromJson(Map<String, dynamic> json) => ActivitySummary(
        steps: json['steps'],
        standingMinutes: json['standing_minutes'],
        postureGoalPercentage: (json['posture_goal_percentage'] as num).toDouble(),
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