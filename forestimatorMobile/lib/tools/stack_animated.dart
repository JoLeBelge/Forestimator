import 'dart:async';
import 'package:fforestimator/globals.dart' as gl;
import 'package:flutter/material.dart';

class ForestimatorStack {
  static List<StackAnimated> stack = [];
  static Map<String, void Function(void Function())> animStates = {};

  List<Widget> get widgets => List<Widget>.generate(stack.length, (index) {
    return stack[index];
  });
  List<int> get priorities => List<int>.generate(stack.length, (index) {
    return stack[index].priority;
  });
  List<String> get ids => List<String>.generate(stack.length, (index) {
    return stack[index].id;
  });
  int indexPriority(int priority) =>
      stack.isEmpty
          ? 0
          : priorities.indexWhere((int p) {
            return (p <= priority);
          });

  void add(String id, Widget it, Duration duration, Offset onScreen, Offset offScreen, {int? priority}) {
    if (ids.contains(id)) return;
    stack.insert(
      indexPriority(priority ?? stack.length * 10),
      StackAnimated(
        positions: <Offset>[Offset(offScreen.dx, offScreen.dy), onScreen, offScreen],
        priority: priority ?? stack.length * 10,
        id: id,
        duration: duration,
        child: it,
      ),
    );
    _startTimer(id);
  }

  void pop(String id) {
    if (stack.isEmpty || !ids.contains(id)) return;
    StackAnimated it = stack.firstWhere((StackAnimated a) {
      return a.id == id;
    });
    animStates[id]!(() {
      it.positions[0] = it.positions[2];
    });
    Timer(it.duration, () {
      gl.refreshStack(() {
        stack.removeWhere((StackAnimated a) {
          return a.id == id;
        });
        animStates.remove(id);
      });
    });
  }

  void _startTimer(String id) {
    Timer(Duration(milliseconds: 10), () {
      StackAnimated it = stack.firstWhere((StackAnimated a) {
        return a.id == id;
      });
      if (animStates[id] != null) {
        animStates[id]!(() {
          it.positions[0] = it.positions[1];
        });
      } else {
        _startTimer(id);
      }
    });
  }
}

class StackAnimated extends StatefulWidget {
  const StackAnimated({
    super.key,
    required this.child,
    required this.positions,
    required this.priority,
    required this.id,
    required this.duration,
    this.data,
  });
  final Widget child;
  final List<Offset> positions;
  final int priority;
  final String id;
  final dynamic data;
  final Duration duration;

  @override
  State<StatefulWidget> createState() => _StackAnimated();
}

class _StackAnimated extends State<StackAnimated> {
  @override
  void initState() {
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    ForestimatorStack.animStates[widget.id] = (void Function() it) {
      if (mounted) {
        setState(() {
          it();
        });
      }
    };
    return Container(
      alignment: AlignmentGeometry.center,
      width: double.infinity,
      height: double.infinity,
      child: AnimatedContainer(
        width: gl.eqPx * gl.eqPxW,
        height: gl.eqPx * gl.eqPxH,
        duration: widget.duration,
        alignment: AlignmentGeometry.xy(gl.dsp.alignX(widget.positions[0].dx), gl.dsp.alignY(widget.positions[0].dy)),
        child: widget.child,
      ),
    );
  }
}
