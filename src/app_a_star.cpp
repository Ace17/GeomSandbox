// Copyright (C) 2022 - Vivien Bonnet
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// A* (pronounced "A-Star") algorithm for shortest path

#include <algorithm> // std::find_if
#include <climits>
#include <cstdio> // snprintf
#include <set>
#include <vector>

#include "algorithm_app.h"
#include "random.h"

namespace
{
int manhattanDistance(const Vec2& position1, const Vec2& position2)
{
  return std::abs(position1.x - position2.x) + std::abs(position1.y - position2.y);
}

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

  int distanceFromEnd(int node) const
  {
    const Vec2& pos = nodes[node].pos;
    const Vec2& endPos = nodes[endNode].pos;
    return manhattanDistance(pos, endPos);
  }
};

struct VisitedGraph
{
  std::vector<bool> isVisited;
  std::vector<int> provenance;
  std::vector<int> cost;

  void visitNode(int index, int visitedProvenance, int visitedCost)
  {
    isVisited[index] = true;
    provenance[index] = visitedProvenance;
    cost[index] = visitedCost;
  }
};

int getNodeValue(const Graph& graph, const VisitedGraph visited, int node)
{
  return visited.cost[node] + graph.distanceFromEnd(node);
}

struct NodeValueSorter
{
  const Graph& graph;
  const VisitedGraph& visited;

  bool operator()(int nodeA, int nodeB) const
  {
    const int nodeAValue = getNodeValue(graph, visited, nodeA);
    const int nodeBValue = getNodeValue(graph, visited, nodeB);
    if(nodeAValue != nodeBValue)
      return nodeAValue < nodeBValue;
    return graph.distanceFromEnd(nodeA) < graph.distanceFromEnd(nodeB);
  };
};

using NodeSet = std::multiset<int, NodeValueSorter>;
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
      nodes[currentIndex].pos = Vec2(x, y);
      nodes[currentIndex].renderPos = Vec2((x - (width - 1) / 2.f), (y - (height - 1) / 2.f)) * spacing;

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

void drawVisitedGraph(IDrawer* drawer, const Graph& graph, const VisitedGraph& visited, const NodeSet& nodesToVisit)
{
  auto& nodes = graph.nodes;
  for(int i = 0; i < (int)nodes.size(); ++i)
  {
    if(visited.cost[i] == INT_MAX)
      continue;

    auto isNode = [i](int index) { return i == index; };
    const bool highlighted = std::find_if(nodesToVisit.begin(), nodesToVisit.end(), isNode) != nodesToVisit.end();
    const Color color = highlighted ? Green : White;
    char buffer[32];
    if(highlighted)
    {
      drawer->circle(nodes[i].renderPos, 1.2, color);
      sprintf(buffer, "%d", getNodeValue(graph, visited, i));
    }
    else
    {
      sprintf(buffer, "%d", visited.cost[i]);
    }

    drawer->text(nodes[i].renderPos, buffer, color);

    if(visited.provenance[i] != i)
    {
      const int prov = visited.provenance[i];
      drawer->line(nodes[prov].renderPos, nodes[i].renderPos, White);
    }
  }
}

struct AStarAlgorithm
{
  static Graph generateInput()
  {
    const int width = 10;
    const int height = 8;
    return randomGraph(width, height);
  }

  static Output execute(Graph input)
  {
    auto& nodes = input.nodes;
    const Vec2& endPos = nodes[input.endNode].pos;
    const int nodesCount = (int)nodes.size();

    VisitedGraph visited;
    visited.isVisited.resize(nodesCount, false);
    visited.provenance.resize(nodesCount, INT_MAX);
    visited.cost.resize(nodesCount, INT_MAX);

    const NodeValueSorter sorter = {input, visited};
    NodeSet nodesToVisit(sorter);

    nodesToVisit.insert(input.startNode);
    visited.visitNode(input.startNode, input.startNode, 0);
    while(!nodesToVisit.empty() && *nodesToVisit.begin() != input.endNode)
    {
      const int nodeIndex = *nodesToVisit.begin();
      const Node& node = nodes[nodeIndex];
      const int nodeCost = visited.cost[nodeIndex];
      nodesToVisit.erase(nodesToVisit.begin());

      drawVisitedGraph(gVisualizer, input, visited, nodesToVisit);
      gVisualizer->circle(node.renderPos, 1.2, Red);
      gVisualizer->step();

      for(int neighbor : node.neighbours)
      {
        if(!visited.isVisited[neighbor])
        {
          const Vec2& neighborRenderPos = nodes[neighbor].renderPos;
          const int neighborCost = nodeCost + 1;
          const int neighborDistanceFromEnd = input.distanceFromEnd(neighbor);
          drawVisitedGraph(gVisualizer, input, visited, nodesToVisit);
          gVisualizer->circle(node.renderPos, 1.2, Red);
          gVisualizer->circle(neighborRenderPos, 1.2, Green);
          gVisualizer->line(node.renderPos, neighborRenderPos, Green);
          char buffer[32];
          sprintf(buffer, "%d+%d=%d", neighborDistanceFromEnd, neighborCost, neighborDistanceFromEnd + neighborCost);
          gVisualizer->text(neighborRenderPos, buffer, Green);
          gVisualizer->step();

          visited.visitNode(neighbor, nodeIndex, nodeCost + 1);
          nodesToVisit.insert(neighbor);
        }
      }
    }

    if(!nodesToVisit.empty())
    {
      std::vector<int> result;
      int node = input.endNode;
      result.push_back(node);
      while(node != input.startNode)
      {
        node = visited.provenance[node];
        result.push_back(node);
      }
      return result;
    }

    // No path found.
    return {};
  }

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

  static void drawOutput(IDrawer* drawer, const Graph& input, const Output& output)
  {
    auto& nodes = input.nodes;
    for(int i = 0; i < (int)output.size(); ++i)
    {
      const int node = output[i];
      if(node != input.startNode && node != input.endNode)
        drawer->circle(nodes[node].renderPos, 1.2, Green);
      if(i > 0)
      {
        const int previousNode = output[i - 1];
        drawer->line(input.nodes[node].renderPos, input.nodes[previousNode].renderPos, Green);
      }
    }
  }
};

const int reg = registerApp("AStar", []() -> IApp* { return new AlgorithmApp<AStarAlgorithm>; });
}
