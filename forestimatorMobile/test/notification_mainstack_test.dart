import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/tools/notification.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  setUp(() async {
    // ensure a clean stack and stable display values
    gl.mainStack.clear();
    gl.dsp = gl.Display.empty();
    gl.dsp.width = 1200;
    gl.dsp.height = 1600;
    gl.dsp.dpi = 1.0;
    gl.dsp.orientation = Orientation.portrait;
    gl.dsp.equipixel = gl.dsp.height / gl.minEquiPixelsDisplayPortraitHeight;
  });

  testWidgets('presentPopup uses mainStack when notificationContext is set', (WidgetTester tester) async {
    // Prepare a minimal app and capture a BuildContext to use as notificationContext
    BuildContext? rootContext;
    await tester.pumpWidget(
      MaterialApp(
        home: Builder(
          builder: (context) {
            rootContext = context;
            return Container();
          },
        ),
      ),
    );

    // Sanity
    expect(rootContext, isNotNull);

    // Set notificationContext so presentPopup will push to gl.mainStack
    gl.notificationContext = rootContext;

    final int before = gl.mainStack.length;

    // Call presentPopup using a simple dialog widget.
    presentPopup(popup: popupNoInternet(), context: rootContext);

    // The mainStack should have been increased by one synchronously.
    expect(gl.mainStack.length, before + 1);
    expect(gl.mainStack.last.runtimeType, equals(Stack));

    // Now dismiss using the public dismissPopup API and verify the stack shrinks.
    dismissPopup();
    expect(gl.mainStack.length, before);
  });
}
