import 'package:go_router/go_router.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;

// Stateful nested navigation based on:
// https://github.com/flutter/packages/blob/main/packages/go_router/example/lib/stateful_shell_route.dart
class ScaffoldWithNestedNavigation extends StatefulWidget {
  const ScaffoldWithNestedNavigation({Key? key, required this.navigationShell})
      : super(key: key ?? const ValueKey('ScaffoldWithNestedNavigation'));
  final StatefulNavigationShell navigationShell;
  @override
  State<StatefulWidget> createState() => _ScaffoldWithNestedNavigation();
}

class _ScaffoldWithNestedNavigation
    extends State<ScaffoldWithNestedNavigation> {
  void _goBranch(int index) {
    widget.navigationShell.goBranch(
      index,
      // A common pattern when using bottom navigation bars is to support
      // navigating to the initial location when tapping the item that is
      // already active. This example demonstrates how to support this behavior,
      // using the initialLocation parameter of goBranch.
      initialLocation: index == widget.navigationShell.currentIndex,
    );
  }

  void rebuildBarForOfflineMode() {
    setState(() {});
  }

  @override
  void initState() {
    gl.rebuildNavigatorBar = rebuildBarForOfflineMode;
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: widget.navigationShell,
      bottomNavigationBar: NavigationBar(
        height: MediaQuery.of(context).size.height * 0.07,
        selectedIndex: widget.navigationShell.currentIndex,
        destinations: [
          const NavigationDestination(label: 'Carte', icon: Icon(Icons.map)),
          NavigationDestination(
              label: 'Couches Online',
              icon: const Icon(Icons.layers),
              enabled: !gl.offlineMode),
          const NavigationDestination(
              label: 'Couches Offline', icon: Icon(Icons.offline_bolt))
        ],
        onDestinationSelected: _goBranch,
        backgroundColor: gl.offlineMode ? gl.colorAgroBioTech : gl.colorUliege,
      ),
    );
  }
}
