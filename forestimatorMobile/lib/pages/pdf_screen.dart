import 'package:fforestimator/globals.dart' as gl;
import 'package:flutter/material.dart';
import 'package:flutter_pdfview/flutter_pdfview.dart';
import 'dart:async';

class PDFScreen extends StatefulWidget {
  final String path;
  final String titre;
  final int currentPage;

  const PDFScreen({
    super.key,
    required this.path,
    required this.titre,
    this.currentPage = 0,
  });
  @override
  State<StatefulWidget> createState() => _PDFScreenState();
}

class _PDFScreenState extends State<PDFScreen> with WidgetsBindingObserver {
  final Completer<PDFViewController> _controller =
      Completer<PDFViewController>();
  int? pages = 0;
  bool isReady = false;
  String errorMessage = '';

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.titre),
        actions: <Widget>[
          IconButton(icon: Icon(Icons.share), onPressed: () {}),
        ],
      ),
      body: Stack(
        children: <Widget>[
          PDFView(
            filePath: widget.path,
            enableSwipe: true,
            swipeHorizontal: true,
            autoSpacing: false,
            pageFling: true,
            pageSnap: true,
            defaultPage:
                widget.currentPage > 0 ? widget.currentPage : gl.currentPage,
            fitPolicy: FitPolicy.BOTH,
            preventLinkNavigation:
                false, // if set to true the link is handled in flutter
            onRender: (xpages) {
              setState(() {
                pages = xpages;
                isReady = true;
              });
            },
            onError: (error) {
              setState(() {
                errorMessage = error.toString();
              });
              gl.print(error.toString());
            },
            onPageError: (page, error) {
              setState(() {
                errorMessage = '$page: ${error.toString()}';
              });
              gl.print('$page: ${error.toString()}');
            },
            onViewCreated: (PDFViewController pdfViewController) {
              _controller.complete(pdfViewController);
            },
            onPageChanged: (int? page, int? total) {
              setState(() {
                gl.currentPage = page!;
              });
            },
          ),
          errorMessage.isEmpty
              ? !isReady
                  ? Center(child: CircularProgressIndicator())
                  : Container()
              : Center(child: Text(errorMessage)),
        ],
      ),
      floatingActionButton: FutureBuilder<PDFViewController>(
        future: _controller.future,
        builder: (context, AsyncSnapshot<PDFViewController> snapshot) {
          if (snapshot.hasData) {
            return FloatingActionButton.extended(
              label: Text("Aller à la page ${pages! ~/ 2}"),
              onPressed: () async {
                await snapshot.data!.setPage(pages! ~/ 2);
              },
            );
          }

          return Container();
        },
      ),
    );
  }
}
