class LayerTile {
  LayerTile({
    required this.name,
    required this.filter,
    this.isExpanded = false,
  });

  String name;
  String filter;
  bool isExpanded;
}