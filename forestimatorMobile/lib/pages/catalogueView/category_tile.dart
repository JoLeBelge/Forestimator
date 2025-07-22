class Category {
  Category({
    required this.name,
    required this.filter,
    this.isExpanded = false,
  });

  String name;
  String filter;
  bool isExpanded;
}