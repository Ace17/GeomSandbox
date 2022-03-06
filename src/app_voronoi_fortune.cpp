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

const BoundingBox box {{-20, -10}, {20, 10}};

struct Arc
{
  Vec2 site;
  Arc* left = nullptr;
  Arc* right = nullptr;

  float pointOn(float x, float lineY) const
  {
    return 1 / (2 * (site.y - lineY)) * powf(x - site.x, 2.f) + (site.y + lineY) / 2.f;
  };
};

void drawArc(IDrawer* drawer, const Arc* arc, float lineY, Color color)
{
  const Vec2 site = arc->site;
  const float stepSize = 1.f;
  const int stepCount = 20;
  const float startX = site.x - stepCount / 2 * stepSize;
  const float endX = site.x + stepCount / 2 * stepSize;
  for(float x = startX; x < endX; x += stepSize)
  {
    const float nextX = x + stepSize;
    drawer->line({x, arc->pointOn(x, lineY)}, {nextX, arc->pointOn(nextX, lineY)}, color);
  }
}

void drawLine(IDrawer* drawer, const Arc* rightArc, const Arc* leftArc, Color color)
{
  const Vec2 linePerpendicular = leftArc->site - rightArc->site;
  const Vec2 direction = Vec2(-linePerpendicular.y, linePerpendicular.x);
  const Vec2 origin = ((leftArc->site + rightArc->site) / 2.f);
  drawer->line(origin, origin + direction * 1000.f, color);
}

class Event;
struct CompareEvents;
using EventQueue = std::priority_queue<Event*, std::vector<Event*>, CompareEvents>;
struct Event
{
  virtual ~Event() = default;
  virtual Vec2 pos() const = 0;
  virtual void happen(EventQueue& eventQueue, Arc*& rootArc) = 0;
};

struct CompareEvents
{
  bool operator()(Event* eventA, Event* eventB) { return eventA->pos().y < eventB->pos().y; }
};

Arc* findAboveArc(Arc* rootArc, Vec2 pos)
{
  // TODO arcs are sorted, we could implement a binary search.
  Arc* arc = rootArc;
  Arc* lowestArc = arc;
  float lowestArcPositionY = arc->pointOn(pos.x, pos.y);
  while(arc->right)
  {
    arc = arc->right;
    const float arcPositionY = arc->pointOn(pos.x, pos.y);
    if(arcPositionY < lowestArcPositionY)
    {
      lowestArcPositionY = arcPositionY;
      lowestArc = arc;
    }
  }
  return lowestArc;
}

struct siteEvent final : public Event
{
  Vec2 Pos;

  siteEvent(Vec2 pos)
      : Pos(pos)
  {
  }

  Vec2 pos() const override { return Pos; }
  void happen(EventQueue& eventQueue, Arc*& rootArc) override
  {
    Arc* newArc = new Arc({Pos});
    if(!rootArc)
      rootArc = newArc;
    else
    {
      Arc* aboveArc = findAboveArc(rootArc, Pos);
      Arc* newLeftArc = new Arc({aboveArc->site});
      Arc* newRightArc = new Arc({aboveArc->site});

      gVisualizer->line(Pos, {Pos.x, aboveArc->pointOn(Pos.x, Pos.y)}, Green);
      drawArc(gVisualizer, aboveArc, Pos.y, Green);
      drawLine(gVisualizer, newLeftArc, newArc, Green);
      drawLine(gVisualizer, newArc, newRightArc, Green);

      newArc->left = newLeftArc;
      newArc->right = newRightArc;
      newLeftArc->left = aboveArc->left;
      newLeftArc->right = newArc;
      newRightArc->left = newArc;
      newRightArc->right = aboveArc->right;

      if(aboveArc->left)
      {
        aboveArc->left->right = newLeftArc;
      }
      if(aboveArc->right)
      {
        aboveArc->right->left = newRightArc;
      }
      delete aboveArc;

      rootArc = newLeftArc;
      while(rootArc->left)
      {
        rootArc = rootArc->left;
      }
    }
  }
};

void drawHorizontalLine(IDrawer* drawer, Vec2 point, Color color)
{
  const Vec2 topPoint = {box.min.x, point.y};
  const Vec2 bottomPoint = {box.max.x, point.y};
  drawer->line(topPoint, bottomPoint, color);
}

void drawBeachLine(IDrawer* drawer, const Arc* rootArc, float lineY, Color color)
{
  if(rootArc)
  {
    const Arc* arc = rootArc;
    drawArc(drawer, arc, lineY, color);
    while(arc->right)
    {
      drawLine(drawer, arc, arc->right, color);
      drawArc(drawer, arc->right, lineY, color);
      arc = arc->right;
    }
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
    Arc* rootArc = nullptr;
    for(Vec2 point : input)
    {
      eventQueue.push(new siteEvent(point));
    }

    while(!eventQueue.empty())
    {
      Event* event = eventQueue.top();
      eventQueue.pop();
      const Vec2 eventPos = event->pos();
      drawBeachLine(gVisualizer, rootArc, eventPos.y, Yellow);
      drawHorizontalLine(gVisualizer, eventPos, Red);
      event->happen(eventQueue, rootArc);
      gVisualizer->step();
      delete event;
    }

    while(rootArc)
    {
      Arc* arc = rootArc;
      rootArc = rootArc->right;
      delete arc;
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
