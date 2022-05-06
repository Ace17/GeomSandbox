// Copyright (C) 2022 - Vivien Bonnet
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// A* (pronounced "A-Star") algorithm for shortest path

#include <cstdio> // snprintf
#include <vector>

#include "algorithm_app.h"
#include "random.h"

namespace
{
struct Node
{
  Vec2 pos;
  Vec2 renderPos; // For display only
  std::vector<int> neighbours;
};

struct Graph
{
  std::vector<Node> nodes;
  int startNode;
  int endNode;
};

using Output = std::vector<int>;

Graph randomGraph(int width, int height)
{
  const float connectionProbability = 0.7f;
  const float spacing = 4.2f;

  Graph graph;
  auto& nodes = graph.nodes;

  graph.nodes.resize(width * height);

  auto connect = [&nodes](int id1, int id2)
  {
    nodes[id1].neighbours.push_back(id2);
    nodes[id2].neighbours.push_back(id1);
  };

  for(int y = 0; y < height; ++y)
  {
    for(int x = 0; x < width; ++x)
    {
      const int currentIndex = y * width + x;
      nodes[currentIndex].pos = Vec2((x - (width - 1) / 2.f), (y - (height - 1) / 2.f));
      nodes[currentIndex].renderPos = nodes[currentIndex].pos * spacing;

      if(x > 0 && randomFloat(0.f, 1.f) < connectionProbability)
      {
        connect(currentIndex, currentIndex - 1);
      }
      if(y > 0 && randomFloat(0.f, 1.f) < connectionProbability)
      {
        connect(currentIndex, currentIndex - width);
      }
    }
  }

  graph.startNode = randomInt(0, width * height);
  graph.endNode = randomInt(0, width * height);
  while(graph.endNode == graph.startNode)
    graph.endNode = randomInt(0, width * height);

  return graph;
}

int manhattanDistance(const Vec2& position1, const Vec2& position2)
{
  return std::abs(position1.x - position2.x) + std::abs(position1.y - position2.y);
}

struct AStarAlgorithm
{
  static Graph generateInput()
  {
    const int width = 10;
    const int height = 8;
    return randomGraph(width, height);
  }

  static Output execute(Graph input) { return {}; }

  static void drawInput(IDrawer* drawer, const Graph& input)
  {
    auto& nodes = input.nodes;
    const Vec2& endpos = nodes[input.endNode].pos;

    for(int idx = 0; idx < nodes.size(); ++idx)
    {
      auto& node = nodes[idx];
      drawer->circle(node.renderPos, 0.5, Gray);

      for(int neighbor : node.neighbours)
        drawer->line(nodes[idx].renderPos, nodes[neighbor].renderPos, Gray);

      char buffer[32];
      sprintf(buffer, "%d", manhattanDistance(node.pos, endpos));

      drawer->text(node.renderPos, buffer, Gray);
    }

    drawer->circle(nodes[input.startNode].renderPos, 1.2, Yellow);
    drawer->circle(nodes[input.endNode].renderPos, 1.2, LightBlue);
  }

  static void drawOutput(IDrawer* drawer, const Graph& input, const Output& output) {}
};

const int reg = registerApp("AStar", []() -> IApp* { return new AlgorithmApp<AStarAlgorithm>; });
}
