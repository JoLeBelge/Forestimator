import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/tools/notification.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('popupSettingsMenu opens and Fermer closes', (tester) async {
    await tester.binding.setSurfaceSize(const Size(1200, 1600));
    gl.display = gl.Display.empty();
    gl.display.width = 1200;
    gl.display.height = 1600;
    gl.display.dpi = 1.0;
    gl.display.orientation = Orientation.portrait;
    gl.display.equipixel =
        gl.display.height / gl.minEquiPixelsDisplayPortraitHeight;

    bool closed = false;
    await tester.pumpWidget(
      MaterialApp(
        home: Builder(
          builder: (context) {
            return ElevatedButton(
              onPressed: () {
                /*showDialog(
                  context: context,
                  builder:
                      (c) => opupSettingsMenu(c, 'name', () {}, () {
                        closed = true;
                      }),
                );*/
              },
              child: const Text('OpenSettings'),
            );
          },
        ),
      ),
    );

    await tester.tap(find.text('OpenSettings'));
    await tester.pumpAndSettle();

    // 'Fermer' button exists
    expect(find.text('Fermer'), findsOneWidget);

    // Ensure mainStack has an entry so mainStackPopLast does not throw
    gl.mainStack.add(Container());

    await tester.tap(find.text('Fermer'));
    await tester.pumpAndSettle();

    expect(closed, isTrue);
  });

  testWidgets('popupPDFSaved shows name and calls after', (tester) async {
    await tester.binding.setSurfaceSize(const Size(1200, 1600));
    gl.display = gl.Display.empty();
    gl.display.width = 1200;
    gl.display.height = 1600;
    gl.display.dpi = 1.0;
    gl.display.orientation = Orientation.portrait;
    gl.display.equipixel =
        gl.display.height / gl.minEquiPixelsDisplayPortraitHeight;

    bool afterCalled = false;
    await tester.pumpWidget(
      MaterialApp(
        home: Builder(
          builder: (context) {
            return ElevatedButton(
              onPressed: () {
                showDialog(
                  context: context,
                  builder:
                      (c) => popupPDFSaved('mydoc.pdf', () {
                        afterCalled = true;
                      }),
                );
              },
              child: const Text('OpenPdfSaved'),
            );
          },
        ),
      ),
    );

    await tester.tap(find.text('OpenPdfSaved'));
    await tester.pumpAndSettle();

    // Title contains the pdf name
    expect(find.text('Export du pdf: mydoc.pdf'), findsOneWidget);

    // Tap the OK button to trigger the after callback
    expect(find.text('OK'), findsOneWidget);
    await tester.tap(find.text('OK'));
    await tester.pumpAndSettle();

    expect(afterCalled, isTrue);
  });
}
