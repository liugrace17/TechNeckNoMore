import 'dart:convert';
import 'package:http/http.dart' as http;
import '../models/models.dart';

class ApiService {
  static const String baseUrl = 'http://localhost:8000';

  static Future<ActivitySummary> getActivitySummary() async {
    final response = await http.get(Uri.parse('$baseUrl/activity/summary'));
    if (response.statusCode == 200) {
      return ActivitySummary.fromJson(jsonDecode(response.body));
    }
    throw Exception('Failed to load activity summary');
  }

  static Future<List<GpsPoint>> getGpsRoute() async {
    final response = await http.get(Uri.parse('$baseUrl/gps/route'));
    if (response.statusCode == 200) {
      final List<dynamic> data = jsonDecode(response.body);
      return data.map((e) => GpsPoint.fromJson(e)).toList();
    }
    throw Exception('Failed to load GPS route');
  }
}