#include "split_polyhedron.h"

#include <unordered_map>

namespace std
{
template<>
struct hash<Vec3>
{
  size_t operator()(const Vec3& pos) const
  {
    static auto floatHash = [](float v) { return std::hash<float>{}(v); };
    return floatHash(pos.x * 7) ^ floatHash(pos.y * 5) ^ floatHash(pos.z * 13);
  }
};
}

namespace
{
int getVertex(Vec3 pos, Polyhedron3f& poly, std::unordered_map<Vec3, int>& cache)
{
  if(cache.empty())
  {
    for(int i = 0; i < (int)poly.vertices.size(); ++i)
      cache[poly.vertices[i]] = i;
  }

  auto it = cache.find(pos);
  if(it == cache.end())
  {
    const int i = poly.vertices.size();
    poly.vertices.push_back(pos);
    it = cache.insert(std::make_pair(pos, i)).first;
  }

  return it->second;
}

int rollover(int value, int period)
{
  while(value >= period)
    value -= period;
  return value;
}

}

void splitPolyhedronAgainstPlane(const Polyhedron3f& poly, Plane3 plane, Polyhedron3f& front, Polyhedron3f& back)
{
  front = {};
  back = {};

  std::unordered_map<Vec3, int> cacheFront;
  std::unordered_map<Vec3, int> cacheBack;

  struct Edge
  {
    Vec3 a, b;
  };

  for(auto& face : poly.faces)
  {
    std::vector<Vec3> verticesFront;
    std::vector<Vec3> verticesBack;

    for(int i = 0; i < (int)face.indices.size(); ++i)
    {
      int i0 = face.indices[i];
      int i1 = face.indices[rollover(i + 1, face.indices.size())];

      const Vec3 v0 = poly.vertices[i0];
      const Vec3 v1 = poly.vertices[i1];

      const int side0 = dotProduct(v0, plane.normal) > plane.dist ? +1 : -1;
      const int side1 = dotProduct(v1, plane.normal) > plane.dist ? +1 : -1;

      if(side0 == 1)
        verticesFront.push_back(v0);
      else
        verticesBack.push_back(v0);

      if(side0 != side1)
      {
        const Vec3 tangent = normalize(v1 - v0);
        const Vec3 vI = v0 + tangent * (plane.dist - dotProduct(v0, plane.normal)) / dotProduct(tangent, plane.normal);

        if(side0 == 1)
        {
          verticesFront.push_back(vI);
          verticesBack.push_back(vI);
        }
        else
        {
          verticesBack.push_back(vI);
          verticesFront.push_back(vI);
        }
      }
    }

    if(verticesFront.size())
    {
      front.faces.push_back({});
      auto& face = front.faces.back();
      for(auto& v : verticesFront)
        face.indices.push_back(getVertex(v, front, cacheFront));
    }

    if(verticesBack.size())
    {
      back.faces.push_back({});
      auto& face = back.faces.back();
      for(auto& v : verticesBack)
        face.indices.push_back(getVertex(v, back, cacheBack));
    }
  }
}
