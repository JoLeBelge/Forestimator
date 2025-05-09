import 'package:flutter/material.dart';

class PopupNotification extends StatelessWidget {
  final String? title;
  final String? dialog;
  final String? accept;
  final String? decline;
  final Function? onAccept;
  final Function? onDecline;
  const PopupNotification({
    super.key,
    this.title,
    this.accept,
    this.dialog,
    this.onAccept,
    this.decline,
    this.onDecline,
  });

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      title: Text(title!),
      content: Text(dialog!),
      actions: <Widget>[
        TextButton(
          onPressed:
              () => {
                Navigator.pop(context, 'OK'),
                if (onAccept != null) onAccept!(),
              },
          child: Text(accept!),
        ),
        if (decline != null)
          TextButton(
            onPressed:
                () => {
                  Navigator.pop(context, 'NO'),
                  if (onDecline != null) onDecline!(),
                },
            child: Text(decline!),
          ),
      ],
    );
  }
}

class PopupNoInternet extends StatelessWidget {
  const PopupNoInternet({super.key});

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      title: Text("Oups"),
      content: Text("Vous n'avez pas accès à internet."),
      actions: <Widget>[
        TextButton(
          child: Text("OK"),
          onPressed: () {
            Navigator.of(context, rootNavigator: true).pop();
          },
        ),
      ],
    );
  }
}

class PopupNotification2 extends StatelessWidget {
  final String? title;
  final String? dialog;
  final String? accept;
  final String? decline;
  final Function? onAccept;
  final Function? onDecline;
  const PopupNotification2({
    super.key,
    this.title,
    this.accept,
    this.dialog,
    this.onAccept,
    this.decline,
    this.onDecline,
  });

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      title: Text(title!),
      content: Text(dialog!),
      actions: <Widget>[
        TextButton(
          onPressed:
              () => {
                if (onAccept != null) onAccept!(),
              },
          child: Text(accept!),
        ),
        if (decline != null)
          TextButton(
            onPressed:
                () => {
                  if (onDecline != null) onDecline!(),
                },
            child: Text(decline!),
          ),
      ],
    );
  }
}
