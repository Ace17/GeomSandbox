#include "bvh.h"

#include "core/sandbox.h"

#include <algorithm>

namespace
{
BoundingBox computeBoundingBox(span<const Triangle> allTriangles, span<const int> triangles)
{
  BoundingBox r;

  for(auto& tri : triangles)
  {
    r.add(allTriangles[tri].a);
    r.add(allTriangles[tri].b);
    r.add(allTriangles[tri].c);
  }

  return r;
}

void subdivide(int nodeIdx, span<const Triangle> allTriangles, std::vector<BvhNode>& nodes)
{
  auto node = &nodes[nodeIdx];
  Vec2 size = node->boundaries.max - node->boundaries.min;
  Vec2 cuttingNormal;

  if(size.x > size.y)
    cuttingNormal = Vec2(1, 0);
  else
    cuttingNormal = Vec2(0, 1);

  auto byDistanceToCuttingPlane = [&](int i, int j)
  { return allTriangles[i].a * cuttingNormal < allTriangles[j].a * cuttingNormal; };

  std::sort(node->triangles.begin(), node->triangles.end(), byDistanceToCuttingPlane);

  const int middle = node->triangles.size() / 2;

  {
    auto linePos = allTriangles[node->triangles[middle]].a;
    auto cuttingDir = rotateLeft(cuttingNormal);
    sandbox_line(linePos - cuttingDir * 100, linePos + cuttingDir * 100, Green);
    sandbox_rect(node->boundaries.min, node->boundaries.max - node->boundaries.min, Red);
    sandbox_breakpoint();
  }

  node->children[0] = nodes.size();
  nodes.push_back({});
  auto& child0 = nodes.back();
  child0.triangles.assign(node->triangles.begin(), node->triangles.begin() + middle);
  child0.boundaries = computeBoundingBox(allTriangles, child0.triangles);

  node->children[1] = nodes.size();
  nodes.push_back({});
  auto& child1 = nodes.back();
  child1.triangles.assign(node->triangles.begin() + middle, node->triangles.end());
  child1.boundaries = computeBoundingBox(allTriangles, child1.triangles);

  node->triangles.clear();

  {
    auto a = &nodes[node->children[0]];
    auto b = &nodes[node->children[1]];
    sandbox_rect(a->boundaries.min, a->boundaries.max - a->boundaries.min, Green);
    sandbox_rect(b->boundaries.min, b->boundaries.max - b->boundaries.min, Green);
    sandbox_breakpoint();
  }
}
}

std::vector<BvhNode> computeBoundingVolumeHierarchy(span<const Triangle> triangles)
{
  std::vector<BvhNode> nodes;
  nodes.reserve(triangles.len * 2);

  nodes.push_back({});
  auto& rootNode = nodes.back();
  for(int i = 0; i < (int)triangles.len; ++i)
    rootNode.triangles.push_back(i);
  rootNode.boundaries = computeBoundingBox(triangles, rootNode.triangles);

  std::vector<int> stack;
  stack.push_back(0);

  while(stack.size())
  {
    auto curr = stack.back();
    stack.pop_back();

    if(nodes[curr].triangles.size() <= 2)
      continue;

    subdivide(curr, triangles, nodes);

    stack.push_back(nodes[curr].children[1]);
    stack.push_back(nodes[curr].children[0]);
  }

  nodes.shrink_to_fit();
  return nodes;
}
