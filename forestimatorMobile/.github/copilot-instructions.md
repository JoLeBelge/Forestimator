## Quick context

This repository is a Flutter mobile app (package name: `fforestimator`) for viewing and downloading map layers and producing reports for forest stations in Wallonia. The UI is native Flutter (widgets + many AlertDialogs/popups), with a heavy reliance on global state in `lib/globals.dart`.

## Quick commands

- Install deps: `flutter pub get`
- Run on connected device/emulator: `flutter run` (or use `flutter run -d <id>`)
- Build APK: `flutter build apk`
- Build iOS: open `ios/Runner.xcworkspace` in Xcode, run `pod install` if needed, then build from Xcode
- Static analysis: `flutter analyze` (project uses `flutter_lints`)

## Run tests & CI checks

- Install deps (always before running tests): `flutter pub get`
- Run unit & widget tests: `flutter test`
- Run a single test file (expanded output): `flutter test test/notification_widget_test.dart -r expanded`
- Run all tests with coverage (machine may need lcov tooling): `flutter test --coverage`

Note: tests live in the `test/` folder. Some tests exercise widgets and rely on mocked globals or small amounts of state set in `setUp()` blocks.

## Big-picture architecture

- UI: Flutter widgets under `lib/` (pages in `lib/pages`, reusable controls in `lib/tools`). Many large UI-building files exist (example: `lib/tools/notification.dart`).
- Global state: `lib/globals.dart` contains most shared flags, dimensions, callbacks (e.g., `refreshMainStack`, `rebuildLayerSwitcher`), and domain lists (layers, polygons, selected layers). Prefer invoking the provided global callback hooks to trigger UI updates rather than broad refactors.
- Domain data: `lib/dico/` holds domain dictionaries (layer metadata, species, etc.). Layer definitions and metadata drives map/catalog behaviour.
- Persistence & integrations: uses `assets/db/fforestimator.db` (packaged DB), `shared_preferences`, `sqflite`, and downloads via `flutter_downloader` and remote API endpoints (e.g., `https://forestimator.gembloux.ulg.ac.be`).

## Project-specific conventions & patterns

- Equipixel/responsive sizing: UI sizes use an "equipixel" abstraction set in `lib/globals.dart` (`Display` class). Use that for sizing to match existing UI scale.
- Global mutable callbacks: UI redraw and cross-widget communications are done by assigning functions to globals (e.g., `gl.rebuildLayerSwitcher = (f) { ... }`). When changing behaviour, update the correct global setter instead of adding new ad-hoc rebuild paths.
- Layer codes: layers are referenced by short keys (`mCode`, `mGroupe`). Look up metadata in `lib/dico/*` rather than hardcoding strings.
- Long single-file widgets: many popup menus and large composite widgets live in single files (notably `lib/tools/notification.dart`). Make small, localized edits and prefer extracting helpers only when necessary.
- Naming: many fields use `m` prefix (e.g., `mCode`, `mNom`) and French UI strings — preserve these conventions for readability.

## Integration & external dependencies

- Remote tile/raster API: `queryApiRastDownload` in `lib/globals.dart` points to the raster service used for downloads. Replacing endpoints requires coordinated changes with the downloader code in `lib/tools/layer_downloader.dart`.
- Database & assets: packaged DB at `assets/db/fforestimator.db` and PDFs under `assets/pdf/`. If you change DB schema, update initial assets and any migration logic.
- Native/platform: app uses `permission_handler`, `geolocator`, `flutter_downloader` (Android/iOS permissions and setup). For iOS builds, confirm `Podfile` settings in `ios/` and run `pod install`.

## Safe-edit guidance for AI agents

- Avoid sweeping refactors of `lib/globals.dart` or replacing global callback patterns; these are central to UI flow and tests are not present to guard changes.
- When changing layout sizes, use `Display.equipixel` and related constants in `lib/globals.dart` to remain consistent.
- If adding imports, use the package import style already used (e.g., `package:fforestimator/...`).
- To update or add a popup or dialog, prefer editing the existing widget functions (for example `popupSearchMenu` / `SearchMenu` in `lib/tools/notification.dart`) and use `gl.refreshMainStack()` or the provided rebuild callbacks to trigger UI changes.

- When changing code that affects UI state, run `flutter test` locally to validate behavior. If tests fail after a change, prefer making small, localized fixes and updating tests' expectations rather than large refactors.
- Tests in this repo are primarily unit/widget tests — avoid modifying `lib/globals.dart` structure unless absolutely necessary. If you must change global APIs, update and run the test suite and add migration notes in this file.

## Guidance for AI when working on tests

- Start by running `flutter pub get` then `flutter test` to see the current failures.
- Prefer to fix tests by adjusting the minimal amount of production code or by updating test setup/mocks to reflect intended behavior.
- Keep tests deterministic: avoid relying on network, time-based, or platform-dependent behaviors. Use dependency injection or mock implementations where possible.
- When adding tests, follow existing style in `test/` and keep tests small (one logical assertion per test). Add at least one happy-path unit/widget test and one edge-case test for any new functionality.

## Last updated

- 2025-11-03: Added test instructions and AI guidance for working with the test suite and globals.

## Where to look for common tasks

- Main entry and app wiring: `lib/main.dart`
- Shared UI values and global callbacks: `lib/globals.dart`
- Layer metadata and dictionaries: `lib/dico/` (look for `dico_apt.dart`)
- Large UI/tooling file that contains many dialogs: `lib/tools/notification.dart`
- Layer downloader and offline logic: `lib/tools/layer_downloader.dart`

If anything here is unclear or you'd like the instructions tuned (more examples, expanded build notes, or a short list of safe refactor targets), tell me what to add and I will update this file.
