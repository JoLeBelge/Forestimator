import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/tools/notification.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  group('switchRowColWithOrientation', () {
    testWidgets('returns Column for portrait orientation', (
      WidgetTester tester,
    ) async {
      // Arrange
      gl.display.orientation = Orientation.portrait;

      // Act
      final widget = switchRowColWithOrientation([
        const Text('A'),
        const Text('B'),
      ]);

      // Assert
      expect(widget, isA<Column>());
    });

    testWidgets('returns Row for landscape orientation', (
      WidgetTester tester,
    ) async {
      // Arrange
      gl.display.orientation = Orientation.landscape;

      // Act
      final widget = switchRowColWithOrientation([
        const Text('A'),
        const Text('B'),
      ]);

      // Assert
      expect(widget, isA<Row>());
    });

    test('styleSettingMenu returns expected font size', () {
      // Arrange
      final expected = gl.display.equipixel * gl.fontSizeM;

      // Act
      final style = styleSettingMenu();

      // Assert
      expect(style, isA<TextStyle>());
      expect(style.fontSize, expected);
    });

    test('stroke returns a Column with three children', () {
      // Act
      final w = stroke(1.0, 2.0, Colors.red);

      // Assert
      expect(w, isA<Column>());
      final col = w as Column;
      expect(col.children.length, 3);
    });
  });
}
