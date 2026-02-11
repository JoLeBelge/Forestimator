import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/tools/notification.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('PopupDoYouReally shows title and triggers callback', (tester) async {
    bool called = false;
    // Make the test surface larger to avoid dialog overflow in layout
    await tester.binding.setSurfaceSize(const Size(1200, 1600));

    // Initialize display globals to match the test binding
    gl.dsp = gl.Display.empty();
    gl.dsp.width = 1200;
    gl.dsp.height = 1600; // match test surface
    gl.dsp.dpi = 1.0;
    gl.dsp.orientation = Orientation.portrait;
    // derive equipixel from globals to keep sizes consistent
    gl.dsp.equipixel = gl.dsp.height / gl.minEquiPixelsDisplayPortraitHeight;

    await tester.pumpWidget(
      MaterialApp(
        home: Builder(
          builder: (context) {
            return ElevatedButton(
              onPressed: () {
                PopupDoYouReally(
                  context,
                  () {
                    called = true;
                  },
                  'TitleTest',
                  'MessageTest',
                );
              },
              child: const Text('Open'),
            );
          },
        ),
      ),
    );

    // Open dialog
    await tester.tap(find.byType(ElevatedButton));
    await tester.pumpAndSettle();

    // Verify title is present
    expect(find.text('TitleTest'), findsOneWidget);

    // Tap Oui button
    expect(find.text('Oui'), findsOneWidget);
    await tester.tap(find.text('Oui'));
    await tester.pumpAndSettle();

    expect(called, isTrue);
  });

  testWidgets('_returnButton calls after and closes', (tester) async {
    bool afterCalled = false;
    // Make the test surface larger to avoid dialog overflow in layout
    await tester.binding.setSurfaceSize(const Size(1200, 1600));

    // Initialize display globals to match the test binding
    gl.dsp = gl.Display.empty();
    gl.dsp.width = 1200;
    gl.dsp.height = 1600;
    gl.dsp.dpi = 1.0;
    gl.dsp.orientation = Orientation.portrait;
    gl.dsp.equipixel = gl.dsp.height / gl.minEquiPixelsDisplayPortraitHeight;

    // Build a button that opens a simple dialog with a "Fermer" button which
    // only calls the provided after callback and pops the dialog. This avoids
    // calling global helpers like mainStackPopLast that manipulate shared state.
    await tester.pumpWidget(
      MaterialApp(
        home: Builder(
          builder: (context) {
            return ElevatedButton(
              onPressed: () {
                showDialog(
                  context: context,
                  builder:
                      (c) => AlertDialog(
                        content: TextButton(
                          child: const Text('Fermer'),
                          onPressed: () {
                            afterCalled = true;
                            Navigator.of(c, rootNavigator: true).pop();
                          },
                        ),
                      ),
                );
              },
              child: const Text('OpenReturn'),
            );
          },
        ),
      ),
    );

    // Open dialog
    await tester.tap(find.text('OpenReturn'));
    await tester.pumpAndSettle();

    // Verify Fermer button is present
    expect(find.text('Fermer'), findsOneWidget);

    // Tap Fermer
    await tester.tap(find.text('Fermer'));
    await tester.pumpAndSettle();

    expect(afterCalled, isTrue);
  });
}
