import 'dart:math';

/// Small epsilon for floating point comparisons
const double _geoEps = 1e-9;

/// Check if two points are equal (with epsilon)
bool _pointsEqual(Point a, Point b, [double eps = _geoEps]) {
  return (a.x - b.x).abs() <= eps && (a.y - b.y).abs() <= eps;
}

int orientation(Point p, Point q, Point r, [double eps = _geoEps]) {
  final val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
  if (val.abs() <= eps) return 0; // colinear
  return (val > 0) ? 1 : 2; // clockwise or counterclockwise
}

bool onSegment(Point p, Point q, Point r, [double eps = _geoEps]) {
  // Check if q lies on segment pr (inclusive)
  return q.x <= max(p.x, r.x) + eps &&
      q.x >= min(p.x, r.x) - eps &&
      q.y <= max(p.y, r.y) + eps &&
      q.y >= min(p.y, r.y) - eps;
}

bool doIntersect(
  Point p1,
  Point q1,
  Point p2,
  Point q2, [
  double eps = _geoEps,
]) {
  final o1 = orientation(p1, q1, p2, eps);
  final o2 = orientation(p1, q1, q2, eps);
  final o3 = orientation(p2, q2, p1, eps);
  final o4 = orientation(p2, q2, q1, eps);

  if (o1 != o2 && o3 != o4) return true;

  if (o1 == 0 && onSegment(p1, p2, q1, eps)) return true;
  if (o2 == 0 && onSegment(p1, q2, q1, eps)) return true;
  if (o3 == 0 && onSegment(p2, p1, q2, eps)) return true;
  if (o4 == 0 && onSegment(p2, q1, q2, eps)) return true;

  return false;
}

bool isPolygonWellDefined(List<Point> polygon, [double eps = _geoEps]) {
  if (polygon.length < 3) return true;

  // Remove consecutive duplicate points (and normalize duplicates)
  List<Point> pts = [];
  for (var p in polygon) {
    if (pts.isEmpty || !_pointsEqual(pts.last, p, eps)) {
      pts.add(p);
    }
  }

  // Also check if first and last are duplicates (closing point) and remove the last one
  if (pts.length > 1 && _pointsEqual(pts.first, pts.last, eps)) {
    pts.removeLast();
  }

  if (pts.length < 3) return true;

  final n = pts.length;

  // Check for any zero-length edge
  for (int i = 0; i < n; i++) {
    final a = pts[i];
    final b = pts[(i + 1) % n];
    if (_pointsEqual(a, b, eps)) return false; // degenerate
  }

  for (int i = 0; i < n; i++) {
    for (int j = i + 1; j < n; j++) {
      // Edges are [i, i+1] and [j, j+1]
      if (i == j) continue;
      final nextI = (i + 1) % n;
      final nextJ = (j + 1) % n;

      // Skip same or adjacent edges
      if (nextI == j || nextJ == i) continue;

      final p1 = pts[i];
      final q1 = pts[nextI];
      final p2 = pts[j];
      final q2 = pts[nextJ];

      if (doIntersect(p1, q1, p2, q2, eps)) {
        return false;
      }
    }
  }
  return true;
}
