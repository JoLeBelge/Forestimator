class LayerTile {
  LayerTile({
    required this.key,
    required this.name,
    required this.filter,
    this.isExpanded = false,
    this.selected = false,
  });

  String key;
  String name;
  String filter;
  bool isExpanded;
  bool selected;
}