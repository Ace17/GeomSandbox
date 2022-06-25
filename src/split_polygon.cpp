// Split a concave polygon along a line
#include "split_polygon.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>

#include "sandbox.h"

static constexpr auto epsilon = 0.01f;

void addFace(Polygon2f& poly, Vec2 a, Vec2 b)
{
  if(magnitude(a - b) < epsilon)
    return;

  const auto i0 = (int)poly.vertices.size();
  poly.vertices.push_back(a);
  poly.vertices.push_back(b);
  poly.faces.push_back({i0 + 0, i0 + 1});
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

  auto byTangentCoord = [tangentCut](Vec2 a, Vec2 b)
  { return dot_product(a, tangentCut) < dot_product(b, tangentCut); };

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
    const auto dist = dot_product(v, plane.normal) - plane.dist;
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

    const auto dist_a = dot_product(a, plane.normal) - plane.dist;
    const auto dist_b = dot_product(b, plane.normal) - plane.dist;

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
