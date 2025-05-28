import 'package:flutter/material.dart';
import 'package:flutter_colorpicker/flutter_colorpicker.dart';

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

class PopupPermissions extends StatelessWidget {
  final String? title;
  final String? dialog;
  final String? accept;
  final String? decline;
  final Function? onAccept;
  final Function? onDecline;
  const PopupPermissions({
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
          onPressed: () => {if (onAccept != null) onAccept!()},
          child: Text(accept!),
        ),
        if (decline != null)
          TextButton(
            onPressed: () => {if (onDecline != null) onDecline!()},
            child: Text(decline!),
          ),
      ],
    );
  }
}

class PopupDownloadSuccess {
  PopupDownloadSuccess(BuildContext context, String layerName) {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text("Téléchargement de $layerName."),
          content: Text("$layerName a été téléchargée avec succès."),
          actions: [
            TextButton(
              child: Text("OK"),
              onPressed: () {
                Navigator.of(context, rootNavigator: true).pop();
              },
            ),
          ],
        );
      },
    );
  }
}

class PopupDownloadFailed {
  PopupDownloadFailed(BuildContext context, String layerName) {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text("Téléchargement de $layerName."),
          content: Text("$layerName n'a pas été téléchargé."),
          actions: [
            TextButton(
              child: Text("OK"),
              onPressed: () {
                Navigator.of(context, rootNavigator: true).pop();
              },
            ),
          ],
        );
      },
    );
  }
}

class PopupPDFSaved {
  PopupPDFSaved(BuildContext context, String pdfName) {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text("Export du pdf: $pdfName"),
          content: Text("Export effectué avec succès."),
          actions: [
            TextButton(
              child: Text("OK"),
              onPressed: () {
                Navigator.of(context, rootNavigator: true).pop();
              },
            ),
          ],
        );
      },
    );
  }
}

class PopupColorChooser {
  Color pickerColor = Color(0xff443a49);

  PopupColorChooser(
    Color currentColor,
    BuildContext context,
    Function(Color) colorChange,
    Function after,
  ) {
    pickerColor = currentColor;
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text("Choisisez une couleur!"),
          content: SingleChildScrollView(
            child: ColorPicker(
              pickerColor: pickerColor,
              onColorChanged: colorChange,
            ),
          ),
          actions: [
            TextButton(
              child: Text("OK"),
              onPressed: () {
                currentColor = pickerColor;
                after();
                Navigator.of(context, rootNavigator: true).pop();
              },
            ),
          ],
        );
      },
    );
  }
}

class PopupNameIntroducer {
  PopupNameIntroducer(
    BuildContext context,
    String currentName,
    Function(String) state,
    Function after,
  ) {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text("Changez de nom"),
          content: SingleChildScrollView(
            child: TextFormField(initialValue: currentName, onChanged: state),
          ),
          actions: [
            TextButton(
              child: Text("Rename"),
              onPressed: () {
                after();
                Navigator.of(context, rootNavigator: true).pop();
              },
            ),
          ],
        );
      },
    );
  }
}
