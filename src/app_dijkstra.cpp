// Copyright (C) 2022 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Dijkstra's algorithm for shortest path

#include <climits>
#include <cmath>
#include <cstdio> // snprintf
#include <set>
#include <vector>

#include "algorithm_app.h"
#include "random.h"

namespace
{
struct Node
{
  struct Neighbor
  {
    int id;
    int cost = 1;
  };
  Vec2 pos; // for display
  std::vector<Neighbor> neighboors;
};

struct Graph
{
  std::vector<Node> nodes;
};

struct DijkstraAlgorithm
{
  static Graph generateInput()
  {
    Graph r;

    auto& nodes = r.nodes;

    static const int N = 7;
    static const auto spacing = 4.2f;

    auto getId = [](int x, int y) { return x + y * N; };

    nodes.resize(N * N);

    for(int y = 0; y < N; ++y)
    {
      for(int x = 0; x < N; ++x)
      {
        Node& n = nodes[getId(x, y)];
        n.pos = Vec2((x - N / 2) * spacing, (y - N / 2) * spacing);

        if(x % 2)
          n.pos.y += spacing * 0.2;

        if(y % 2)
          n.pos.x += spacing * 0.2;

        if(x > 0)
          n.neighboors.push_back({getId(x - 1, y)});
        if(y > 0)
          n.neighboors.push_back({getId(x, y - 1)});
        if(x < N - 1)
          n.neighboors.push_back({getId(x + 1, y)});
        if(y < N - 1)
          n.neighboors.push_back({getId(x, y + 1)});
      }
    }

    return r;
  }

  static std::vector<int> execute(Graph input)
  {
    auto& nodes = input.nodes;

    std::vector<bool> isVisited(nodes.size());
    std::vector<int> cost(nodes.size(), INT_MAX);

    std::set<int> todo; // list of nodes bordering the visited area

    const int StartNode = nodes.size() / 2;

    todo.insert(StartNode);
    cost[StartNode] = 0;

    while(todo.size())
    {
      // find node on TODO list with smallest cost
      int bestId = -1;
      int bestCost = INT_MAX;
      for(auto id : todo)
      {
        if(cost[id] < bestCost)
        {
          bestId = id;
          bestCost = cost[id];
        }
      }

      const auto current = bestId;
      todo.erase(current);

      isVisited[current] = true;

      for(auto& nb : nodes[current].neighboors)
      {
        if(isVisited[nb.id])
          continue;

        const int nbCost = cost[current] + nb.cost;
        if(nbCost >= cost[nb.id])
          continue;

        cost[nb.id] = nbCost;
        todo.insert(nb.id);

        gVisualizer->circle(nodes[nb.id].pos, 1, Red);
        gVisualizer->line(nodes[current].pos, nodes[nb.id].pos, Red);
      }

      gVisualizer->circle(nodes[current].pos, 2, Red);

      for(int i = 0; i < (int)nodes.size(); ++i)
      {
        char buffer[32];
        if(cost[i] == INT_MAX)
          sprintf(buffer, "-");
        else
          sprintf(buffer, "%d", cost[i]);

        const bool highlight = todo.find(i) != todo.end();
        gVisualizer->text(nodes[i].pos, buffer, highlight ? Green : White);
      }

      for(auto id : todo)
        gVisualizer->circle(nodes[id].pos, 1, Green);

      gVisualizer->step();
    }

    std::vector<int> result;

    return result;
  }

  static void drawInput(IDrawer* drawer, const Graph& input)
  {
    auto& nodes = input.nodes;

    for(int idx = 0; idx < nodes.size(); ++idx)
    {
      auto& node = nodes[idx];
      drawer->circle(node.pos, 0.5);

      for(auto& nb : node.neighboors)
        drawer->line(nodes[idx].pos, nodes[nb.id].pos, White);
    }
  }

  static void drawOutput(IDrawer* drawer, const Graph& input, const std::vector<int>& output)
  {
    auto& nodes = input.nodes;

    if(output.empty())
      return;

    int prevIdx = output[0];
    for(auto idx : output)
    {
      drawer->line(nodes[prevIdx].pos, nodes[idx].pos, Green);
      prevIdx = idx;
    }
  }
};

const int reg = registerApp("Dijkstra", []() -> IApp* { return new AlgorithmApp<DijkstraAlgorithm>; });
}
