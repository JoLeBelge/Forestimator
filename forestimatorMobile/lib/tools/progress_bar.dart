import 'package:flutter/material.dart';

class ProgressBar extends StatefulWidget {
  const ProgressBar({super.key});

  @override
  State<ProgressBar> createState() => _ProgressBarState();
}

class _ProgressBarState extends State<ProgressBar> {
  @override
  Widget build(BuildContext context) {
    return Container(
      constraints: BoxConstraints(
          minWidth: MediaQuery.of(context).size.width * 1,
          maxWidth: MediaQuery.of(context).size.width * 1,
          minHeight: MediaQuery.of(context).size.height * .05,
          maxHeight: MediaQuery.of(context).size.height * .05),
      child: LinearProgressIndicator(
        value: 0.5,
        semanticsLabel: 'Linear progress indicator',
      ),
    );
  }
}
