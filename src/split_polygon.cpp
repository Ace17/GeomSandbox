// Split a concave polygon along a line
#include "split_polygon.h"

#include "core/sandbox.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>

static constexpr auto epsilon = 0.01f;

// dedup vertices
int addVertex(Polygon2f& poly, Vec2 a)
{
  const auto N = (int)poly.vertices.size();

  for(int i = 0; i < N; ++i)
  {
    auto delta = poly.vertices[i] - a;
    if(dotProduct(delta, delta) < epsilon * epsilon)
      return i;
  }

  poly.vertices.push_back(a);
  return N;
}

void addFace(Polygon2f& poly, Vec2 a, Vec2 b)
{
  if(magnitude(a - b) < epsilon)
    return;

  const auto ia = addVertex(poly, a);
  const auto ib = addVertex(poly, b);
  poly.faces.push_back({ia, ib});
}

void closePolygon(Polygon2f& poly, Vec2 tangentCut)
{
  struct LessVec2f
  {
    bool operator()(Vec2 a, Vec2 b) const
    {
      if(a.x != b.x)
        return a.x < b.x;
      return a.y < b.y;
    }
  };

  std::map<Vec2, int, LessVec2f> points;
  for(auto face : poly.faces)
  {
    points[poly.vertices[face.a]]++;
    points[poly.vertices[face.b]]++;
  }

  std::vector<Vec2> orphans;
  for(auto& p : points)
  {
    assert(p.second == 1 || p.second == 2);
    if(p.second == 1)
      orphans.push_back(p.first);
  }

  auto byTangentCoord = [tangentCut](Vec2 a, Vec2 b) { return dotProduct(a, tangentCut) < dotProduct(b, tangentCut); };

  std::sort(orphans.begin(), orphans.end(), byTangentCoord);

  assert(orphans.size() % 2 == 0);
  for(int i = 0; i < (int)orphans.size(); i += 2)
    addFace(poly, orphans[i + 0], orphans[i + 1]);
}

void splitPolygonAgainstPlane(const Polygon2f& poly, Plane plane, Polygon2f& front, Polygon2f& back)
{
  const auto T = rotateLeft(plane.normal);

  for(auto v : poly.vertices)
  {
    const auto dist = dotProduct(v, plane.normal) - plane.dist;
    Color c;
    if(dist > epsilon)
      c = LightBlue;
    else if(dist < -epsilon)
      c = Green;
    else
      c = Yellow;

    sandbox_circle(v, 0.2, c);
  }

  for(auto face : poly.faces)
  {
    const auto a = poly.vertices[face.a];
    const auto b = poly.vertices[face.b];

    const auto dist_a = dotProduct(a, plane.normal) - plane.dist;
    const auto dist_b = dotProduct(b, plane.normal) - plane.dist;

    if(std::abs(dist_a) < epsilon && std::abs(dist_b) < epsilon)
    {
      // drop the face. It will be added back by 'closePolygon'
    }
    else if(dist_a >= 0 && dist_b >= 0)
    {
      addFace(front, a, b);
    }
    else if(dist_a <= 0 && dist_b <= 0)
    {
      addFace(back, a, b);
    }
    else // the face is crossing the plane, split it
    {
      const auto intersection = a + (b - a) * (dist_a / (dist_a - dist_b));

      if(dist_a > 0 && dist_b < 0)
      {
        addFace(front, a, intersection);
        addFace(back, intersection, b);
      }
      else
      {
        assert(dist_a < 0 && dist_b > 0);
        addFace(front, intersection, b);
        addFace(back, a, intersection);
      }
    }
  }

  closePolygon(back, T);
  closePolygon(front, -T);
}
