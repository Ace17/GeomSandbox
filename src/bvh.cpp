#include "bvh.h"

#include "core/sandbox.h"

#include <algorithm>

namespace
{
BoundingBox computeBoundingBox(span<const BoundingBox> allObjects, span<const int> objects)
{
  BoundingBox r;

  for(auto& tri : objects)
  {
    r.add(allObjects[tri].min);
    r.add(allObjects[tri].max);
  }

  return r;
}

void subdivide(int nodeIdx, span<const BoundingBox> allObjects, std::vector<BvhNode>& nodes)
{
  auto node = &nodes[nodeIdx];
  Vec2 size = node->boundaries.max - node->boundaries.min;
  Vec2 cuttingNormal;

  if(size.x > size.y)
    cuttingNormal = Vec2(1, 0);
  else
    cuttingNormal = Vec2(0, 1);

  auto byDistanceToCuttingPlane = [&](int i, int j)
  {
    auto& objA = allObjects[i];
    auto& objB = allObjects[j];
    auto centerA = (objA.min + objA.max) * 0.5f;
    auto centerB = (objB.min + objB.max) * 0.5f;
    return centerA * cuttingNormal < centerB * cuttingNormal;
  };

  std::sort(node->objects.begin(), node->objects.end(), byDistanceToCuttingPlane);

  const int middle = node->objects.size() / 2;

  {
    auto linePos = allObjects[node->objects[middle]].min;
    auto cuttingDir = rotateLeft(cuttingNormal);
    sandbox_line(linePos - cuttingDir * 100, linePos + cuttingDir * 100, Green);
    sandbox_rect(node->boundaries.min, node->boundaries.max - node->boundaries.min, Red);
    sandbox_breakpoint();
  }

  node->children[0] = nodes.size();
  nodes.push_back({});
  auto& child0 = nodes.back();
  child0.objects.assign(node->objects.begin(), node->objects.begin() + middle);
  child0.boundaries = computeBoundingBox(allObjects, child0.objects);

  node->children[1] = nodes.size();
  nodes.push_back({});
  auto& child1 = nodes.back();
  child1.objects.assign(node->objects.begin() + middle, node->objects.end());
  child1.boundaries = computeBoundingBox(allObjects, child1.objects);

  node->objects.clear();

  {
    auto a = &nodes[node->children[0]];
    auto b = &nodes[node->children[1]];
    sandbox_rect(a->boundaries.min, a->boundaries.max - a->boundaries.min, Green);
    sandbox_rect(b->boundaries.min, b->boundaries.max - b->boundaries.min, Green);
    sandbox_breakpoint();
  }
}
}

std::vector<BvhNode> computeBoundingVolumeHierarchy(span<const BoundingBox> objects)
{
  std::vector<BvhNode> nodes;
  nodes.reserve(objects.len * 2);

  nodes.push_back({});
  auto& rootNode = nodes.back();
  for(int i = 0; i < (int)objects.len; ++i)
    rootNode.objects.push_back(i);
  rootNode.boundaries = computeBoundingBox(objects, rootNode.objects);

  std::vector<int> stack;
  stack.push_back(0);

  while(stack.size())
  {
    auto curr = stack.back();
    stack.pop_back();

    if(nodes[curr].objects.size() <= 2)
      continue;

    subdivide(curr, objects, nodes);

    stack.push_back(nodes[curr].children[1]);
    stack.push_back(nodes[curr].children[0]);
  }

  nodes.shrink_to_fit();
  return nodes;
}
