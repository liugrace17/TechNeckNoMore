import 'package:flutter/material.dart';

class AppTheme {
  static const Color bg = Color(0xFF0F1117);
  static const Color surface = Color(0xFF1A1D27);
  static const Color border = Color(0xFF2A2D3A);

  static const Color accent = Color(0xFFE8C547);
  static const Color accentGreen = Color(0xFF4CAF7D);
  static const Color accentRed = Color(0xFFE05C5C);
  static const Color accentBlue = Color(0xFF5B9BD5);

  static const Color textPrimary = Color(0xFFEEEFF4);
  static const Color textSecondary = Color(0xFF8B8FA8);
  static const Color textMuted = Color(0xFF555870);

  static ThemeData get theme => ThemeData(
        brightness: Brightness.dark,
        scaffoldBackgroundColor: bg,
        colorScheme: const ColorScheme.dark(
          primary: accent,
          surface: surface,
        ),
        fontFamily: 'monospace',
      );
}