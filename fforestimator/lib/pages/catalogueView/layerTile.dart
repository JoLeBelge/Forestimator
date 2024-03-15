class LayerTile {
  LayerTile({
    required this.key,
    required this.name,
    required this.filter,
    this.downloadable = false,
    this.isExpanded = false,
    this.selected = false,
    this.extern = false,
  });

  String key;
  String name;
  String filter;
  bool isExpanded;
  bool selected;
  bool extern;
  bool downloadable;
}