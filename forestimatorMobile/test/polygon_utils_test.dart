import 'dart:math';

import 'package:flutter_test/flutter_test.dart';
import 'package:fforestimator/tools/geometry/polygon_utils.dart' as polygon;

void main() {
  test('empty polygon is well defined', () {
    expect(polygon.isPolygonWellDefined([]), true);
  });

  test('triangle is well defined', () {
    final tri = [Point(0, 0), Point(1, 0), Point(0, 1)];
    expect(polygon.isPolygonWellDefined(tri), true);
  });

  test('square is well defined', () {
    final square = [Point(0, 0), Point(1, 0), Point(1, 1), Point(0, 1)];
    expect(polygon.isPolygonWellDefined(square), true);
  });

  test('self-intersecting polygon (hourglass) is not well defined', () {
    final hourglass = [Point(0, 0), Point(1, 1), Point(0, 1), Point(1, 0)];
    expect(polygon.isPolygonWellDefined(hourglass), false);
  });

  test(
    'adding a point on an existing edge returns well defined (colinear)',
    () {
      final withPointOnEdge = [
        Point(0, 0),
        Point(1, 0),
        Point(2, 0),
        Point(1, 1),
      ];
      expect(polygon.isPolygonWellDefined(withPointOnEdge), true);
    },
  );

  test('polygon touching non-adjacent vertex is not allowed', () {
    final poly = [Point(0, 0), Point(2, 0), Point(2, 2), Point(0, 2)];
    // A segment crossing to a vertex
    final touch = [
      Point(0, 0),
      Point(2, 0),
      Point(1, 2),
      Point(2, 2),
      Point(0, 2),
    ];
    expect(polygon.isPolygonWellDefined(poly), true);
    expect(polygon.isPolygonWellDefined(touch), false);
  });

  test('polygon with duplicate points is invalid/normalized', () {
    final dup = [Point(0, 0), Point(1, 0), Point(1, 0), Point(0, 1)];
    // After removing duplicate consecutive points, this is still a triangle -> valid
    expect(polygon.isPolygonWellDefined(dup), true);
  });

  test('closed polygon with first==last is treated correctly', () {
    final closed = [Point(0, 0), Point(1, 0), Point(0, 1), Point(0, 0)];
    expect(polygon.isPolygonWellDefined(closed), true);
  });
}
