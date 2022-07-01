// Copyright (C) 2022 - Vivien Bonnet
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Voronoi diagram: Fortune algorithm

#include "core/algorithm_app.h"
#include "core/sandbox.h"

#include <algorithm>
#include <cmath> // powf
#include <vector>

#include "bounding_box.h"
#include "random.h"

namespace
{

const float screenMinX = -35.f;
const float screenMaxX = 35.f;

template<typename T>
struct optional
{
  bool set = false;
  T value;

  optional<T>& operator=(const T& newValue)
  {
    set = true;
    value = newValue;
    return *this;
  }
};

float clamp(float value, float min, float max) { return std::min(max, std::max(min, value)); }

struct Arc
{
  Vec2 site;
  Arc* left = nullptr;
  Arc* right = nullptr;

  float pointOn(float x, float lineY) const
  {
    return 1 / (2 * (site.y - lineY)) * powf(x - site.x, 2.f) + (site.y + lineY) / 2.f;
  };

  // Return arc as equation y = ax^2 + bx + c
  void asEquation(float lineY, float& a, float& b, float& c) const
  {
    const float d = (2.f * (site.y - lineY));
    a = 1.f / d;
    b = -2.f * site.x / d;
    c = lineY + d / 4.f + site.x * site.x / d;
  }

  static float intersection(const Arc* leftArc, const Arc* rightArc, float lineY)
  {
    if(leftArc->site.y == lineY)
      return leftArc->site.x;
    if(rightArc->site.y == lineY)
      return rightArc->site.x;

    float leftA, leftB, leftC;
    leftArc->asEquation(lineY, leftA, leftB, leftC);
    float rightA, rightB, rightC;
    rightArc->asEquation(lineY, rightA, rightB, rightC);

    const float a = leftA - rightA;
    const float b = leftB - rightB;
    const float c = leftC - rightC;
    const float det = b * b - 4.f * a * c;
    const float x1 = (-b + sqrt(det)) / (2 * a);
    const float x2 = (-b - sqrt(det)) / (2 * a);

    return x1;
  }

  std::pair<float, float> getArcExtremities(float lineY) const
  {
    std::pair<float, float> result = {screenMinX, screenMaxX};

    if(left)
    {
      result.first = Arc::intersection(left, this, lineY);
      result.first = clamp(result.first, screenMinX, screenMaxX);
    }
    if(right)
    {
      result.second = Arc::intersection(this, right, lineY);
      result.second = clamp(result.second, screenMinX, screenMaxX);
    }

    return result;
  }
};

struct Edge
{
  const Arc* leftArc;
  const Arc* rightArc;

  Vec2 origin(float lineY) const
  {
    const float intersectionX = Arc::intersection(leftArc, rightArc, lineY);
    const float intersectionY =
          leftArc->site.y == lineY ? rightArc->pointOn(intersectionX, lineY) : leftArc->pointOn(intersectionX, lineY);
    return Vec2(intersectionX, intersectionY);
  }

  Vec2 direction() const
  {
    const Vec2 linePerpendicular = leftArc->site - rightArc->site;
    return Vec2(-linePerpendicular.y, linePerpendicular.x);
  }

  // Return line as equation y = ax + b
  void asEquation(float& a, float& b, float lineY) const
  {
    const Vec2 edgeA = origin(lineY);
    const Vec2 edgeB = edgeA + direction();
    a = (edgeB.y - edgeA.y) / (edgeB.x - edgeA.x);
    b = edgeA.y - a * edgeA.x;
  }
};

struct VoronoiDiagram
{
  struct CellEdge
  {
    Vec2 siteA;
    Vec2 siteB;

    optional<Vec2> vertexA;
    optional<Vec2> vertexB;

    void setVertex(const Vec2& vertex)
    {
      if(!vertexA.set)
        vertexA = vertex;
      else
        vertexB = vertex;
    }
  };

  CellEdge& createEdge(const Vec2& siteA, const Vec2& siteB)
  {
    edges.push_back({siteA, siteB});
    return edges.back();
  }

  CellEdge& findEdge(const Vec2& siteA, const Vec2& siteB)
  {
    auto isSearchedEdge = [siteA, siteB](const CellEdge& edge)
    { return (edge.siteA == siteA && edge.siteB == siteB) || (edge.siteA == siteB && edge.siteB == siteA); };
    return *std::find_if(edges.begin(), edges.end(), isSearchedEdge);
  }

  void fillRemainingVertices(const Arc* rootArc)
  {
    const Arc* arc = rootArc;
    while(arc->right)
    {
      Edge edge = {arc, arc->right};
      CellEdge& cellEdge = findEdge(arc->site, arc->right->site);
      const Vec2 origin = (arc->site + arc->right->site) / 2.f;
      const Vec2 direction = edge.direction();
      cellEdge.setVertex(origin + direction * 1000.f);

      arc = arc->right;
    }
  }

  std::vector<CellEdge> edges;
};

void drawArc(const Arc* arc, float lineY, Color color)
{
  const std::pair<float, float> extremities = arc->getArcExtremities(lineY);
  const float startX = extremities.first;
  const float endX = extremities.second;
  if(startX < endX)
  {
    const float stepSize = 0.5f;
    float x = startX - std::abs(std::fmod(startX, stepSize));
    while(x < endX)
    {
      const float nextX = x + stepSize;
      const float lineStartX = std::max(x, startX);
      const float lineEndX = std::min(nextX, endX);
      sandbox_line({lineStartX, arc->pointOn(lineStartX, lineY)}, {lineEndX, arc->pointOn(lineEndX, lineY)}, color);
      x = nextX;
    }
  }
}

void drawLine(const Arc* rightArc, const Arc* leftArc, Color color)
{
  const Vec2 linePerpendicular = leftArc->site - rightArc->site;
  const Vec2 direction = Vec2(-linePerpendicular.y, linePerpendicular.x);
  const Vec2 origin = ((leftArc->site + rightArc->site) / 2.f);
  sandbox_line(origin, origin + direction * 1000.f, color);
}

void drawEdge(const Edge& edge, Color color) { drawLine(edge.rightArc, edge.leftArc, color); }

void drawHorizontalLine(Vec2 point, Color color)
{
  const Vec2 topPoint = {screenMinX, point.y};
  const Vec2 bottomPoint = {screenMaxX, point.y};
  sandbox_line(topPoint, bottomPoint, color);
}

void drawBeachLine(const Arc* rootArc, float lineY, Color color)
{
  if(rootArc)
  {
    const Arc* arc = rootArc;
    drawArc(arc, lineY, color);
    while(arc->right)
    {
      drawLine(arc->right, arc, color);
      drawArc(arc->right, lineY, color);
      arc = arc->right;
    }
  }
}

void drawDiagram(const VoronoiDiagram& diagram, Color color)
{
  for(auto edge : diagram.edges)
  {
    if(edge.vertexA.set && edge.vertexB.set)
      sandbox_line(edge.vertexA.value, edge.vertexB.value, color);
  }
}

class Event;
struct CompareEvents;
using EventQueue = std::vector<Event*>;
struct Event
{
  virtual ~Event() = default;
  virtual Vec2 pos() const = 0;
  virtual void happen(EventQueue& eventQueue, Arc*& rootArc, VoronoiDiagram& diagram) = 0;
  virtual void draw() const = 0;
};

void drawEvents(const EventQueue& eventQueue)
{
  for(const Event* event : eventQueue)
  {
    event->draw();
  }
}

template<class Pred>
void DeleteEventsWithPredicate(EventQueue& eventQueue, Pred& pred)
{
  auto it = eventQueue.begin();
  while(it != eventQueue.end())
  {
    if(pred(*it))
    {
      delete *it;
      it = eventQueue.erase(it);
    }
    else
      it++;
  }
}

Arc* findAboveArc(Arc* rootArc, Vec2 pos)
{
  // TODO arcs are sorted, we could implement a binary search.
  Arc* arc = rootArc;
  std::pair<float, float> arcExtremities = arc->getArcExtremities(pos.y);
  while(pos.x < arcExtremities.first || pos.x > arcExtremities.second)
  {
    arc = arc->right;
    arcExtremities = arc->getArcExtremities(pos.y);
  }
  return arc;
}

void createCircleEventIfAny(EventQueue& eventQueue, Arc* arc, const Edge& leftEdge, const Edge& rightEdge, float lineY);

struct CircleEvent final : public Event
{
  Arc* arc;
  Vec2 intersection;

  Vec2 pos() const override { return intersection; }
  void happen(EventQueue& eventQueue, Arc*& rootArc, VoronoiDiagram& diagram) override
  {
    sandbox_printf("circleEvent at %f;%f\n", intersection.x, intersection.y);

    sandbox_rect(intersection - Vec2(0.2, 0.2), Vec2(0.4, 0.4), LightBlue);
    arc->right->left = arc->left;
    arc->left->right = arc->right;

    auto IsConcernedArc = [&](Event* event)
    {
      const CircleEvent* circleEvent = dynamic_cast<const CircleEvent*>(event);
      if(!circleEvent)
        return false;
      const Arc* circleArc = circleEvent->arc;
      return (circleArc == arc);
    };

    DeleteEventsWithPredicate(eventQueue, IsConcernedArc);

    sandbox_breakpoint();
    drawBeachLine(rootArc, intersection.y, Yellow);
    drawHorizontalLine(intersection, Red);
    drawEvents(eventQueue);
    drawDiagram(diagram, Yellow);

    const float lineY = intersection.y;
    Edge newEdge = {arc->left, arc->right};
    if(arc->left->left)
    {
      Edge leftEdge = {arc->left->left, arc->left};
      createCircleEventIfAny(eventQueue, arc->left, leftEdge, newEdge, lineY);
    }
    if(arc->right->right)
    {
      Edge rightEdge = {arc->right, arc->right->right};
      createCircleEventIfAny(eventQueue, arc->right, newEdge, rightEdge, lineY);
    }

    const Vec2 voronoiVertex = newEdge.origin(lineY);
    VoronoiDiagram::CellEdge& voronoiEdge = diagram.createEdge(arc->left->site, arc->right->site);

    voronoiEdge.setVertex(voronoiVertex);
    auto& leftEdge = diagram.findEdge(arc->site, arc->left->site);
    leftEdge.setVertex(voronoiVertex);
    auto& rightEdge = diagram.findEdge(arc->site, arc->right->site);
    rightEdge.setVertex(voronoiVertex);

    delete arc;
  }

  void draw() const override { sandbox_rect(intersection - Vec2(0.2, 0.2), Vec2(0.4, 0.4), LightBlue); }
};

void createCircleEventIfAny(EventQueue& eventQueue, Arc* arc, const Edge& leftEdge, const Edge& rightEdge, float lineY)
{
  float edge1A, edge1B;
  float edge2A, edge2B;
  leftEdge.asEquation(edge1A, edge1B, lineY);
  rightEdge.asEquation(edge2A, edge2B, lineY);
  if(std::abs(edge1A) == std::abs(edge2A)) // Case of parallel directions.
    return;

  const float intersectionX = (edge2B - edge1B) / (edge1A - edge2A);
  const float intersectionY = edge1A * intersectionX + edge1B;

  const bool isInLeftEdgeDirection = (leftEdge.direction().x < 0 == intersectionX < leftEdge.origin(lineY).x);
  const bool isInRightEdgeDirection = (rightEdge.direction().x < 0 == intersectionX < rightEdge.origin(lineY).x);

  if(isInLeftEdgeDirection && isInRightEdgeDirection)
  {
    const Vec2 intersection = {intersectionX, intersectionY};
    const float circleRadius = magnitude(intersection - arc->site);
    const Vec2 eventPosition = {intersection.x, intersection.y - circleRadius};
    drawEdge(leftEdge, LightBlue);
    drawEdge(rightEdge, LightBlue);
    drawArc(arc, lineY, LightBlue);
    sandbox_rect(intersection - Vec2(0.2, 0.2), Vec2(0.4, 0.4), LightBlue);
    sandbox_rect(eventPosition - Vec2(0.2, 0.2), Vec2(0.4, 0.4), LightBlue);
    CircleEvent* circleEvent = new CircleEvent();
    circleEvent->arc = arc;
    circleEvent->intersection = eventPosition;
    eventQueue.push_back(circleEvent);
  }
}

void createCircleEventIfAny(EventQueue& eventQueue, Arc* arc, float lineY)
{
  Edge leftEdge = {arc->left, arc};
  Edge rightEdge = {arc, arc->right};
  createCircleEventIfAny(eventQueue, arc, leftEdge, rightEdge, lineY);
}

void createCircleEvents(EventQueue& eventQueue, Arc* leftArc, Arc* middleArc, Arc* rightArc)
{
  const float lineY = middleArc->site.y;
  if(leftArc->left)
  {
    createCircleEventIfAny(eventQueue, leftArc, lineY);
  }
  if(rightArc->right)
  {
    createCircleEventIfAny(eventQueue, rightArc, lineY);
  }
}

struct siteEvent final : public Event
{
  Vec2 Pos;

  siteEvent(Vec2 pos)
      : Pos(pos)
  {
  }

  Vec2 pos() const override { return Pos; }
  void happen(EventQueue& eventQueue, Arc*& rootArc, VoronoiDiagram& diagram) override
  {
    sandbox_printf("siteEvent at %f;%f\n", Pos.x, Pos.y);
    Arc* newArc = new Arc({Pos});
    if(!rootArc)
      rootArc = newArc;
    else
    {

      Arc* aboveArc = findAboveArc(rootArc, Pos);
      Arc* newLeftArc = new Arc({aboveArc->site});
      Arc* newRightArc = new Arc({aboveArc->site});

      diagram.createEdge(Pos, aboveArc->site);

      auto IsCircleEventOnAboveArc = [aboveArc](const Event* event)
      {
        const CircleEvent* circleEvent = dynamic_cast<const CircleEvent*>(event);
        return circleEvent && circleEvent->arc == aboveArc;
      };

      DeleteEventsWithPredicate(eventQueue, IsCircleEventOnAboveArc);

      sandbox_line(Pos, {Pos.x, aboveArc->pointOn(Pos.x, Pos.y)}, Green);
      drawArc(aboveArc, Pos.y, Green);
      drawLine(newLeftArc, newArc, Green);
      drawLine(newArc, newRightArc, Green);

      newArc->left = newLeftArc;
      newArc->right = newRightArc;
      newLeftArc->left = aboveArc->left;
      newLeftArc->right = newArc;
      newRightArc->left = newArc;
      newRightArc->right = aboveArc->right;

      sandbox_breakpoint();
      drawBeachLine(rootArc, Pos.y, Yellow);
      drawHorizontalLine(Pos, Red);
      drawEvents(eventQueue);
      drawDiagram(diagram, Yellow);
      createCircleEvents(eventQueue, newLeftArc, newArc, newRightArc);

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

  void draw() const override { sandbox_rect(Pos - Vec2(0.2, 0.2), Vec2(0.4, 0.4), Green); }
};

struct FortuneVoronoiAlgoritm
{
  static std::vector<Vec2> generateInput()
  {
    const Vec2 min = {-20, -15};
    const Vec2 max = {20, 15};

    std::vector<Vec2> r(15);

    for(auto& p : r)
      p = randomPos(min, max);

    return r;
  }

  static VoronoiDiagram execute(std::vector<Vec2> input)
  {
    EventQueue eventQueue;
    VoronoiDiagram diagram;
    Arc* rootArc = nullptr;
    for(Vec2 point : input)
    {
      eventQueue.push_back(new siteEvent(point));
    }

    while(!eventQueue.empty())
    {
      auto compareEvents = [](Event* eventA, Event* eventB) { return eventA->pos().y < eventB->pos().y; };

      std::sort(eventQueue.begin(), eventQueue.end(), compareEvents);
      Event* event = eventQueue.back();
      eventQueue.pop_back();
      const Vec2 eventPos = event->pos();
      drawBeachLine(rootArc, eventPos.y, Yellow);
      drawHorizontalLine(eventPos, Red);
      drawEvents(eventQueue);
      drawDiagram(diagram, Yellow);
      event->happen(eventQueue, rootArc, diagram);
      sandbox_breakpoint();

      delete event;
    }

    diagram.fillRemainingVertices(rootArc);

    while(rootArc)
    {
      Arc* arc = rootArc;
      rootArc = rootArc->right;
      delete arc;
    }

    return diagram;
  }

  static void display(span<const Vec2> input, const VoronoiDiagram& output)
  {
    for(auto& p : input)
    {
      sandbox_rect(p - Vec2(0.2, 0.2), Vec2(0.4, 0.4));
    }
    drawDiagram(output, Yellow);
  }
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<FortuneVoronoiAlgoritm>>()); }
const int reg = registerApp("FortuneVoronoi", &create);
}
