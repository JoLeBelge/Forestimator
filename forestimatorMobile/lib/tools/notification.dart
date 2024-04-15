import 'package:flutter/material.dart';

class PopupNotification extends StatelessWidget {
  final String? title;
  final String? dialog;
  final String? accept;
  final String? decline;
  final Function? onAccept;
  final Function? onDecline;
  const PopupNotification(
      {super.key,
      this.title,
      this.accept,
      this.dialog,
      this.onAccept,
      this.decline,
      this.onDecline});

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      title: Text(title!),
      content: Text(dialog!),
      actions: <Widget>[
        TextButton(
          onPressed: () =>
              {Navigator.pop(context, 'OK'), if (onAccept != null) onAccept!()},
          child: Text(accept!),
        ),
        if (decline != null)
          TextButton(
            onPressed: () => {
              Navigator.pop(context, 'NO'),
              if (onDecline != null) onDecline!()
            },
            child: Text(decline!),
          )
      ],
    );
  }
}
