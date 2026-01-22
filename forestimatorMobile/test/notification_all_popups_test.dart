import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/tools/notification.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  setUp(() async {
    // Ensure consistent display globals for popup layout
    gl.display = gl.Display.empty();
    gl.display.width = 1200;
    gl.display.height = 1600;
    gl.display.dpi = 1.0;
    gl.display.orientation = Orientation.portrait;
    gl.display.equipixel =
        gl.display.height / gl.minEquiPixelsDisplayPortraitHeight;
  });

  group('notification popups smoke tests', () {
    // Helper to find buttons inside the currently shown AlertDialog(s)
    Finder alertButton(String text) => find.descendant(
      of: find.byType(AlertDialog),
      matching: find.widgetWithText(TextButton, text),
    );

    // Helper to find text inside the currently shown AlertDialog(s)
    Finder alertText(String text) => find.descendant(
      of: find.byType(AlertDialog),
      matching: find.text(text),
    );

    Future<void> pressAlertButton(WidgetTester tester, String text) async {
      // Try a real tap first; if the widget is off-screen or obscured the tap
      // may miss. In that case fallback to calling the onPressed handler
      // directly.
      // Narrow the target to the most recently shown AlertDialog, then find
      // the matching button inside it. This avoids tapping buttons belonging
      // to previous dialogs that may still be in the widget tree.
      final List<Element> dialogs =
          tester.elementList(find.byType(AlertDialog)).toList();
      expect(dialogs, isNotEmpty);
      final Element lastDialog = dialogs.last;
      final Finder dialogFinder = find.byElementPredicate(
        (e) => e == lastDialog,
      );
      final Finder f = find.descendant(
        of: dialogFinder,
        matching: find.widgetWithText(TextButton, text),
      );
      expect(f, findsWidgets);
      try {
        await tester.tap(f.first, warnIfMissed: false);
        await tester.pumpAndSettle();
      } catch (_) {
        final TextButton btn = tester.widget<TextButton>(f.first);
        btn.onPressed?.call();
        await tester.pumpAndSettle();
      }
    }

    testWidgets('popupNoInternet and popupPDFSaved', (tester) async {
      // enlarge test surface so popups layout without overflow
      await tester.binding.setSurfaceSize(const Size(1200, 1600));
      gl.display = gl.Display.empty();
      gl.display.width = 1200;
      gl.display.height = 1600;
      gl.display.dpi = 1.0;
      gl.display.orientation = Orientation.portrait;
      gl.display.equipixel =
          gl.display.height / gl.minEquiPixelsDisplayPortraitHeight;
      bool pdfAfter = false;

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
      // Ensure tests use the showDialog fallback so AlertDialog widgets appear
      // in the widget tree where the test can find them.
      gl.notificationContext = null;

      // Open popupNoInternet directly
      gl.mainStack.add(Container());
      presentPopup(popup: popupNoInternet(), context: rootContext);
      await tester.pumpAndSettle();
      expect(alertText('Message'), findsOneWidget);
      expect(alertText("Vous n'avez pas accès à internet."), findsOneWidget);
      expect(alertButton('OK'), findsOneWidget);
      await pressAlertButton(tester, 'OK');

      // Open popupPDFSaved directly
      gl.mainStack.add(Container());
      presentPopup(
        popup: popupPDFSaved('a.pdf', () {
          pdfAfter = true;
        }),
        context: rootContext,
      );
      await tester.pumpAndSettle();
      expect(alertText('Export du pdf: a.pdf'), findsOneWidget);
      await pressAlertButton(tester, 'OK');
      expect(pdfAfter, isTrue);
    });

    testWidgets('download recommended / permissions / download success/fail', (
      tester,
    ) async {
      await tester.binding.setSurfaceSize(const Size(1200, 1600));
      gl.display = gl.Display.empty();
      gl.display.width = 1200;
      gl.display.height = 1600;
      gl.display.dpi = 1.0;
      gl.display.orientation = Orientation.portrait;
      gl.display.equipixel =
          gl.display.height / gl.minEquiPixelsDisplayPortraitHeight;
      bool accept = false;
      bool decline = false;

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

      // PopupDownloadRecomendedLayers
      gl.mainStack.add(Container());
      presentPopup(
        popup: PopupDownloadRecomendedLayers(
          title: 'T',
          dialog: 'D',
          accept: 'Yes',
          decline: 'No',
          onAccept: () => accept = true,
          onDecline: () => decline = true,
        ),
        context: rootContext,
      );
      await tester.pumpAndSettle();
      expect(alertText('T'), findsOneWidget);
      await pressAlertButton(tester, 'Yes');
      expect(accept, isTrue);

      // PopupPermissions
      accept = false;
      decline = false;
      gl.mainStack.add(Container());
      presentPopup(
        popup: PopupPermissions(
          title: 'PT',
          dialog: 'PD',
          accept: 'A',
          decline: 'D',
          onAccept: () => accept = true,
          onDecline: () => decline = true,
        ),
        context: rootContext,
      );
      await tester.pumpAndSettle();
      expect(alertText('PT'), findsOneWidget);
      await pressAlertButton(tester, 'A');
      expect(accept, isTrue);

      // Also test decline
      accept = false;
      decline = false;
      gl.mainStack.add(Container());
      presentPopup(
        popup: PopupPermissions(
          title: 'PT',
          dialog: 'PD',
          accept: 'A',
          decline: 'D',
          onAccept: () => accept = true,
          onDecline: () => decline = true,
        ),
        context: rootContext,
      );
      await tester.pumpAndSettle();
      await pressAlertButton(tester, 'D');
      expect(decline, isTrue);

      // PopupDownloadSuccess
      gl.mainStack.add(Container());
      // Force showDialog fallback in tests
      gl.notificationContext = null;
      PopupDownloadSuccess(rootContext!, 'L1');
      await tester.pumpAndSettle();
      expect(alertText('L1 a été téléchargée avec succès.'), findsOneWidget);
      await pressAlertButton(tester, 'OK');

      // PopupDownloadFailed
      gl.mainStack.add(Container());
      // Force showDialog fallback in tests
      gl.notificationContext = null;
      PopupDownloadFailed(rootContext!, 'L2');
      await tester.pumpAndSettle();
      expect(alertText("L2 n'a pas été téléchargé."), findsOneWidget);
      await pressAlertButton(tester, 'OK');
    });

    testWidgets('color chooser and name introducer and do you really', (
      tester,
    ) async {
      await tester.binding.setSurfaceSize(const Size(1200, 1600));
      gl.display = gl.Display.empty();
      gl.display.width = 1200;
      gl.display.height = 1600;
      gl.display.dpi = 1.0;
      gl.display.orientation = Orientation.portrait;
      gl.display.equipixel =
          gl.display.height / gl.minEquiPixelsDisplayPortraitHeight;
      bool colorAfter = false;
      bool nameAfter = false;
      bool dydAfter = false;

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

      // Color chooser
      gl.mainStack.add(Container());
      // Force showDialog fallback in tests
      gl.notificationContext = null;
      PopupColorChooser(Colors.blue, rootContext!, (c) {}, () {
        colorAfter = true;
      }, () {});
      await tester.pumpAndSettle();
      expect(alertText('Choisissez une couleur!'), findsOneWidget);
      await pressAlertButton(tester, 'OK');
      expect(colorAfter, isTrue);

      // Name introducer
      gl.mainStack.add(Container());
      // Force showDialog fallback in tests
      gl.notificationContext = null;
      PopupNameIntroducer(
        rootContext!,
        'abc',
        (s) {},
        () {
          nameAfter = true;
        },
        () {},
        () {},
      );
      await tester.pumpAndSettle();
      // The name introducer uses an 'Ok' button in the dialog
      expect(alertButton('Ok'), findsWidgets);
      await pressAlertButton(tester, 'Ok');
      expect(nameAfter, isTrue);

      // Do you really
      gl.mainStack.add(Container());
      // Force showDialog fallback in tests
      gl.notificationContext = null;
      PopupDoYouReally(
        rootContext!,
        () {
          dydAfter = true;
        },
        'TDY',
        'MDY',
      );
      await tester.pumpAndSettle();
      expect(alertText('TDY'), findsOneWidget);
      await pressAlertButton(tester, 'Oui');
      expect(dydAfter, isTrue);
    });

    testWidgets('polygon list and settings menu popups', (tester) async {
      await tester.binding.setSurfaceSize(const Size(1200, 1600));
      gl.display = gl.Display.empty();
      gl.display.width = 1200;
      gl.display.height = 1600;
      gl.display.dpi = 1.0;
      gl.display.orientation = Orientation.portrait;
      gl.display.equipixel =
          gl.display.height / gl.minEquiPixelsDisplayPortraitHeight;
      bool after = false; // used to ensure popup can call after
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

      gl.mainStack.add(Container());
      presentPopup(
        popup: popupPolygonListMenu(rootContext!, 'n', (l) {}, () {
          after = true;
        }),
        context: rootContext,
      );
      await tester.pumpAndSettle();
      expect(alertText('Liste des polygones'), findsOneWidget);
      // Close and assert after was called
      await pressAlertButton(tester, 'Fermer');
      expect(after, isTrue);

      gl.mainStack.add(Container());
      presentPopup(
        popup: popupSettingsMenu(rootContext!, 'n', () {}, () {
          after = true;
        }),
        context: rootContext,
      );
      await tester.pumpAndSettle();
      // SettingsMenu no longer uses ExpansionPanelList in the current UI.
      // Verify the settings dialog by checking for the title instead.
      expect(find.text('Paramètres'), findsOneWidget);
      await pressAlertButton(tester, 'Fermer');
      await tester.pumpAndSettle();
    });

    // Note: heavy/popups that interact with gl.dico or complex widgets are skipped:
    // popupSearchMenu, popupLayerSwitcher, popupOnlineMapMenu, popupResultsMenu,
    // popupAnaResultsMenu, PopupPdfMenu (requires layerTile/dico), etc.
  });
}
