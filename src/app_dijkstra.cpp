// Copyright (C) 2022 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Dijkstra's algorithm for shortest path

#include "core/algorithm_app.h"
#include "core/sandbox.h"

#include <climits>
#include <cstdio> // snprintf
#include <set>
#include <vector>

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
  int startNode;
};

struct Output
{
  std::vector<int> provenance;
  std::vector<int> cost;
};

Graph randomGraph()
{
  Graph r;

  auto& nodes = r.nodes;

  static const int N = 7;
  static const auto spacing = 4.2f;

  auto getId = [](int x, int y) { return x + y * N; };

  nodes.resize(N * N);

  // layout nodes for display
  for(int y = 0; y < N; ++y)
  {
    for(int x = 0; x < N; ++x)
    {
      Node& n = nodes[getId(x, y)];
      n.pos = Vec2((x - N / 2) * spacing, (y - N / 2) * spacing);

      if(x % 2)
        n.pos.y += spacing * 0.5;

      if(y % 2)
        n.pos.x += spacing * 0.5;
    }
  }

  // make connections
  auto connect = [&](int id1, int id2)
  {
    const int cost = 1;
    r.nodes[id1].neighboors.push_back({id2, cost});
    r.nodes[id2].neighboors.push_back({id1, cost});
  };

  for(int y = 0; y < N; ++y)
  {
    for(int x = 0; x < N; ++x)
    {
      const auto id = getId(x, y);

      if(x > 0 && randomInt(0, 10) < 8)
        connect(id, getId(x - 1, y));

      if(y > 0 && randomInt(0, 10) < 8)
        connect(id, getId(x, y - 1));
    }
  }

  r.startNode = randomInt(0, r.nodes.size());

  return r;
}

struct DijkstraAlgorithm
{
  static Graph generateInput() { return randomGraph(); }

  static Output execute(Graph input)
  {
    Output r{};

    auto& nodes = input.nodes;

    std::vector<bool> isVisited(nodes.size());
    r.cost.resize(nodes.size(), INT_MAX);
    r.provenance.resize(nodes.size(), INT_MAX);

    std::set<int> todo; // list of nodes bordering the visited area

    todo.insert(input.startNode);
    r.cost[input.startNode] = 0;
    r.provenance[input.startNode] = input.startNode;

    while(todo.size())
    {
      // find node on TODO list with smallest cost
      int bestId = -1;
      int bestCost = INT_MAX;
      for(auto id : todo)
      {
        if(r.cost[id] < bestCost)
        {
          bestId = id;
          bestCost = r.cost[id];
        }
      }

      const auto current = bestId;
      todo.erase(current);

      isVisited[current] = true;

      for(auto& nb : nodes[current].neighboors)
      {
        if(isVisited[nb.id])
          continue;

        const int nbCost = r.cost[current] + nb.cost;
        if(nbCost >= r.cost[nb.id])
          continue;

        r.cost[nb.id] = nbCost;
        r.provenance[nb.id] = current;

        todo.insert(nb.id);

        sandbox_circle(nodes[nb.id].pos, 1, Red);
        sandbox_line(nodes[current].pos, nodes[nb.id].pos, Red);
      }

      sandbox_circle(nodes[current].pos, 2, Red);

      for(int i = 0; i < (int)nodes.size(); ++i)
      {
        if(r.cost[i] == INT_MAX)
          continue;

        char buffer[32];
        sprintf(buffer, "%d", r.cost[i]);

        const bool highlight = todo.find(i) != todo.end();
        sandbox_text(nodes[i].pos, buffer, highlight ? Green : White);

        if(r.provenance[i] != i)
        {
          const int prov = r.provenance[i];
          sandbox_line(input.nodes[prov].pos, input.nodes[i].pos, White);
        }
      }

      for(auto id : todo)
        sandbox_circle(nodes[id].pos, 1, Green);

      sandbox_breakpoint();
    }

    return r;
  }

  static void display(const Graph& input, const Output& output)
  {
    auto& nodes = input.nodes;

    for(int idx = 0; idx < (int)nodes.size(); ++idx)
    {
      auto& node = nodes[idx];
      sandbox_circle(node.pos, 0.5, Gray);

      for(auto& nb : node.neighboors)
        sandbox_line(nodes[idx].pos, nodes[nb.id].pos, Gray);
    }

    sandbox_circle(nodes[input.startNode].pos, 1.2, Yellow);

    if(output.cost.empty())
      return;

    for(int i = 0; i < (int)input.nodes.size(); ++i)
    {
      if(output.cost[i] == INT_MAX)
        continue;

      char buffer[32];
      sprintf(buffer, "%d", output.cost[i]);

      sandbox_text(input.nodes[i].pos, buffer, Green);

      if(output.provenance[i] != i)
      {
        const int prov = output.provenance[i];
        sandbox_line(input.nodes[prov].pos, input.nodes[i].pos, Green);
      }
    }
  }
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<DijkstraAlgorithm>>()); }
const int reg = registerApp("PathFind.Dijkstra", &create);
}
