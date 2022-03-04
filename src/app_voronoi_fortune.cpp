// Copyright (C) 2022 - Vivien Bonnet
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Voronoi diagram: Fortune algorithm

#include <cmath> // powf
#include <queue> // std::priority_queue
#include <vector>

#include "algorithm_app.h"
#include "bounding_box.h"
#include "random.h"

namespace
{

struct VoronoiDiagram
{
  struct Edge
  {
    int a, b;
  };

  std::vector<Vec2> vertices;
  std::vector<Edge> edges;
};

const BoundingBox box{{-20, -10}, {20, 10}};

struct Edge;

struct Arc
{
  Vec2 site;

  float pointOn(float x, float lineY) const
  {
    return 1 / (2 * (site.y - lineY)) * powf(x - site.x, 2.f) + (site.y + lineY) / 2.f;
  };
};
using ArcList = std::vector<Arc>;

void drawArc(IDrawer* drawer, const Arc& arc, float lineY, Color color)
{
  const Vec2 site = arc.site;
  const float stepSize = 1.f;
  const int stepCount = 20;
  const float startX = site.x - stepCount / 2 * stepSize;
  const float endX = site.x + stepCount / 2 * stepSize;
  for(float x = startX; x < endX; x += stepSize)
  {
    const float nextX = x + stepSize;
    drawer->line({x, arc.pointOn(x, lineY)}, {nextX, arc.pointOn(nextX, lineY)}, color);
  }
}

static float dot(Vec2 v, Vec2 w) { return v.x * w.x + v.y * w.y; }

static constexpr float clamp(float value, float min, float max) { return std::min(max, std::max(min, value)); }

class Event;
struct CompareEvents;
using EventQueue = std::priority_queue<Event*, std::vector<Event*>, CompareEvents>;
struct Event
{
  virtual ~Event() = default;
  virtual Vec2 pos() const = 0;
  virtual void happen(EventQueue& eventQueue, ArcList& arcList) = 0;
};

struct CompareEvents
{
  bool operator()(Event* eventA, Event* eventB) { return eventA->pos().y < eventB->pos().y; }
};

struct siteEvent final : public Event
{
  Vec2 Pos;

  siteEvent(Vec2 pos)
      : Pos(pos)
  {
  }

  Vec2 pos() const override { return Pos; }
  void happen(EventQueue& eventQueue, ArcList& arcList) override { arcList.push_back({Pos}); }
};

void drawHorizontalLine(IDrawer* drawer, Vec2 point, Color color)
{
  const Vec2 topPoint = {box.min.x, point.y};
  const Vec2 bottomPoint = {box.max.x, point.y};
  drawer->line(topPoint, bottomPoint, color);
}

void drawArcs(IDrawer* drawer, const ArcList& arcList, float lineY, Color color)
{
  for(const Arc& arc : arcList)
  {
    drawArc(drawer, arc, lineY, color);
  }
}

struct FortuneVoronoiAlgoritm
{
  static std::vector<Vec2> generateInput()
  {
    std::vector<Vec2> r(15);

    for(auto& p : r)
      p = randomPos(box.min, box.max);

    return r;
  }

  static VoronoiDiagram execute(std::vector<Vec2> input)
  {
    EventQueue eventQueue;
    ArcList arcList;
    for(Vec2 point : input)
    {
      eventQueue.push(new siteEvent(point));
    }

    while(!eventQueue.empty())
    {
      Event* event = eventQueue.top();
      eventQueue.pop();
      const Vec2 eventPos = event->pos();
      drawArcs(gVisualizer, arcList, eventPos.y, Yellow);
      drawHorizontalLine(gVisualizer, eventPos, Red);
      gVisualizer->step();
      event->happen(eventQueue, arcList);
      delete event;
    }

    // TODO
    return {};
  }

  static void drawInput(IDrawer* drawer, const std::vector<Vec2>& input)
  {
    drawer->rect(box.min, box.max - box.min);
    for(auto& p : input)
    {
      drawer->rect(p - Vec2(0.2, 0.2), Vec2(0.4, 0.4));
    }
  }

  static void drawOutput(IDrawer* drawer, const std::vector<Vec2>& input, const VoronoiDiagram& output)
  {
    for(auto& edge : output.edges)
      drawer->line(output.vertices[edge.a], output.vertices[edge.b], Green);
  }
};

const int reg = registerApp("FortuneVoronoi", []() -> IApp* { return new AlgorithmApp<FortuneVoronoiAlgoritm>; });
}
