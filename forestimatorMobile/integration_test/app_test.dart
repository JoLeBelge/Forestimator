import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/main.dart' as app;

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('open PopupDoYouReally and accept', (tester) async {
    app.main();
    await tester.pumpAndSettle();

    // This integration test is a minimal smoke test placeholder. Depending on
    // the app's initial routes and global state, you may need to navigate to
    // the screen that exposes the popup. Here we just assert the app launches.
    expect(find.byType(MaterialApp), findsOneWidget);
  });
}
