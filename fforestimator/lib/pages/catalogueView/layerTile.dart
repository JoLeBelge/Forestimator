class LayerTile {
  LayerTile({
    required this.name,
    required this.filter,
    this.isExpanded = false,
    this.selected = false,
  });

  String name;
  String filter;
  bool isExpanded;
  bool selected;
}