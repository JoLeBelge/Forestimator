import 'package:go_router/go_router.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;

class ScaffoldWithNestedNavigation extends StatefulWidget {
  const ScaffoldWithNestedNavigation({Key? key, required this.navigationShell})
    : super(key: key ?? const ValueKey('ScaffoldWithNestedNavigation'));
  final StatefulNavigationShell navigationShell;
  @override
  State<StatefulWidget> createState() => _ScaffoldWithNestedNavigation();
}

class _ScaffoldWithNestedNavigation
    extends State<ScaffoldWithNestedNavigation> {
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
    return Scaffold(body: widget.navigationShell);
  }
}
