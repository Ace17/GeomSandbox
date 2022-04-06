// Copyright (C) 2022 - Vivien Bonnet
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Voronoi diagram: Fortune algorithm

#include <algorithm>
#include <cmath> // powf
#include <vector>

#include "algorithm_app.h"
#include "bounding_box.h"
#include "random.h"

namespace
{

static float magnitude(Vec2 v) { return sqrt(v * v); }

struct VoronoiDiagram
{
  struct Edge
  {
    int a, b;
  };

  std::vector<Vec2> vertices;
  std::vector<Edge> edges;
};

const BoundingBox box{{-20, -15}, {20, 15}};

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
};

float arcIntersection(const Arc* leftArc, const Arc* rightArc, float lineY)
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

std::pair<float, float> getArcExtremities(const Arc* arc, float lineY)
{
  std::pair<float, float> result = {box.min.x, box.max.x};

  if(arc->left)
  {
    result.first = arcIntersection(arc->left, arc, lineY);
    result.first = clamp(result.first, box.min.x, box.max.x);
  }
  if(arc->right)
  {
    result.second = arcIntersection(arc, arc->right, lineY);
    result.second = clamp(result.second, box.min.x, box.max.x);
  }

  return result;
}

void drawArc(IDrawer* drawer, const Arc* arc, float lineY, Color color)
{
  const std::pair<float, float> extremities = getArcExtremities(arc, lineY);
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
      drawer->line({lineStartX, arc->pointOn(lineStartX, lineY)}, {lineEndX, arc->pointOn(lineEndX, lineY)}, color);
      x = nextX;
    }
  }
}

// Return line as equation y = ax + b
void edgeEquation(const Vec2& edgePosition, const Vec2& edgeDirection, float& a, float& b)
{
  const Vec2 edgeA = edgePosition;
  const Vec2 edgeB = edgePosition + edgeDirection;
  a = (edgeB.y - edgeA.y) / (edgeB.x - edgeA.x);
  b = edgeA.y - a * edgeA.x;
}

void edgeAsPositionAndDirection(const Arc* leftArc, const Arc* rightArc, Vec2& position, Vec2& direction, float lineY)
{
  const float intersectionX = arcIntersection(leftArc, rightArc, lineY);
  const float intersectionY =
        leftArc->site.y == lineY ? rightArc->pointOn(intersectionX, lineY) : leftArc->pointOn(intersectionX, lineY);
  position = Vec2(intersectionX, intersectionY);
  const Vec2 linePerpendicular = leftArc->site - rightArc->site;
  direction = Vec2(-linePerpendicular.y, linePerpendicular.x);
}

void drawLine(IDrawer* drawer, const Arc* rightArc, const Arc* leftArc, Color color)
{
  const Vec2 linePerpendicular = leftArc->site - rightArc->site;
  const Vec2 direction = Vec2(-linePerpendicular.y, linePerpendicular.x);
  const Vec2 origin = ((leftArc->site + rightArc->site) / 2.f);
  drawer->line(origin, origin + direction * 1000.f, color);
}

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
      drawLine(drawer, arc->right, arc, color);
      drawArc(drawer, arc->right, lineY, color);
      arc = arc->right;
    }
  }
}

class Event;
struct CompareEvents;
using EventQueue = std::vector<Event*>;
struct Event
{
  virtual ~Event() = default;
  virtual Vec2 pos() const = 0;
  virtual void happen(EventQueue& eventQueue, Arc*& rootArc) = 0;
  virtual void draw(IDrawer* drawer) const = 0;
};

void drawEvents(IDrawer* drawer, const EventQueue& eventQueue)
{
  for(const Event* event : eventQueue)
  {
    event->draw(drawer);
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
  std::pair<float, float> arcExtremities = getArcExtremities(arc, pos.y);
  while(pos.x < arcExtremities.first || pos.x > arcExtremities.second)
  {
    arc = arc->right;
    arcExtremities = getArcExtremities(arc, pos.y);
  }
  return arc;
}

void createCircleEventIfAny(EventQueue& eventQueue,
      Arc* arc,
      Vec2 leftPosition,
      Vec2 leftDirection,
      Vec2 rightPosition,
      Vec2 rightDirection,
      float lineY);

struct CircleEvent final : public Event
{
  Arc* arc;
  Vec2 intersection;

  Vec2 pos() const override { return intersection; }
  void happen(EventQueue& eventQueue, Arc*& rootArc) override
  {
    printf("circleEvent at %f;%f\n", intersection.x, intersection.y);
    gVisualizer->rect(intersection - Vec2(0.2, 0.2), Vec2(0.4, 0.4), LightBlue);
    arc->right->left = arc->left;
    arc->left->right = arc->right;

    auto IsConcernedArc = [&](Event* event) {
      const CircleEvent* circleEvent = dynamic_cast<const CircleEvent*>(event);
      if(!circleEvent)
        return false;
      const Arc* circleArc = circleEvent->arc;
      return (circleArc == arc);
    };

    DeleteEventsWithPredicate(eventQueue, IsConcernedArc);

    gVisualizer->step();
    drawBeachLine(gVisualizer, rootArc, intersection.y, Yellow);
    drawHorizontalLine(gVisualizer, intersection, Red);
    drawEvents(gVisualizer, eventQueue);

    const float lineY = intersection.y;
    Vec2 newEdgePosition, newEdgeDirection;
    edgeAsPositionAndDirection(arc->left, arc->right, newEdgePosition, newEdgeDirection, lineY);
    if(arc->left->left)
    {
      Vec2 leftPosition, leftDirection;
      edgeAsPositionAndDirection(arc->left->left, arc->left, leftPosition, leftDirection, lineY);
      createCircleEventIfAny(
            eventQueue, arc->left, leftPosition, leftDirection, newEdgePosition, newEdgeDirection, lineY);
    }
    if(arc->right->right)
    {
      Vec2 rightPosition, rightDirection;
      edgeAsPositionAndDirection(arc->right, arc->right->right, rightPosition, rightDirection, lineY);
      createCircleEventIfAny(
            eventQueue, arc->right, newEdgePosition, newEdgeDirection, rightPosition, rightDirection, lineY);
    }

    delete arc;
  }

  void draw(IDrawer* drawer) const override { drawer->rect(intersection - Vec2(0.2, 0.2), Vec2(0.4, 0.4), LightBlue); }
};

void createCircleEventIfAny(EventQueue& eventQueue,
      Arc* arc,
      Vec2 leftPosition,
      Vec2 leftDirection,
      Vec2 rightPosition,
      Vec2 rightDirection,
      float lineY)
{
  float edge1A, edge1B;
  float edge2A, edge2B;
  edgeEquation(leftPosition, leftDirection, edge1A, edge1B);
  edgeEquation(rightPosition, rightDirection, edge2A, edge2B);
  if(std::abs(edge1A) == std::abs(edge2A)) // Case of parallel directions.
    return;

  const float intersectionX = (edge2B - edge1B) / (edge1A - edge2A);
  const float intersectionY = edge1A * intersectionX + edge1B;

  const bool isInLeftEdgeDirection = (leftDirection.x < 0 == intersectionX < leftPosition.x);
  const bool isInRightEdgeDirection = (rightDirection.x < 0 == intersectionX < rightPosition.x);

  if(isInLeftEdgeDirection && isInRightEdgeDirection)
  {
    const Vec2 intersection = {intersectionX, intersectionY};
    const float circleRadius = magnitude(intersection - arc->site);
    const Vec2 eventPosition = {intersection.x, intersection.y - circleRadius};
    gVisualizer->line(leftPosition, leftPosition + leftDirection, LightBlue);
    gVisualizer->line(rightPosition, rightPosition + rightDirection, LightBlue);
    drawArc(gVisualizer, arc, lineY, LightBlue);
    gVisualizer->rect(intersection - Vec2(0.2, 0.2), Vec2(0.4, 0.4), LightBlue);
    gVisualizer->rect(eventPosition - Vec2(0.2, 0.2), Vec2(0.4, 0.4), LightBlue);
    CircleEvent* circleEvent = new CircleEvent();
    circleEvent->arc = arc;
    circleEvent->intersection = eventPosition;
    eventQueue.push_back(circleEvent);
  }
}

void createCircleEventIfAny(EventQueue& eventQueue, Arc* arc, float lineY)
{
  Vec2 leftPosition, leftDirection;
  Vec2 rightPosition, rightDirection;
  edgeAsPositionAndDirection(arc->left, arc, leftPosition, leftDirection, lineY);
  edgeAsPositionAndDirection(arc, arc->right, rightPosition, rightDirection, lineY);
  createCircleEventIfAny(eventQueue, arc, leftPosition, leftDirection, rightPosition, rightDirection, lineY);
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
  void happen(EventQueue& eventQueue, Arc*& rootArc) override
  {
    printf("siteEvent at %f;%f\n", Pos.x, Pos.y);
    Arc* newArc = new Arc({Pos});
    if(!rootArc)
      rootArc = newArc;
    else
    {
      Arc* aboveArc = findAboveArc(rootArc, Pos);
      Arc* newLeftArc = new Arc({aboveArc->site});
      Arc* newRightArc = new Arc({aboveArc->site});

      auto IsCircleEventOnAboveArc = [aboveArc](const Event* event) {
        const CircleEvent* circleEvent = dynamic_cast<const CircleEvent*>(event);
        return circleEvent && circleEvent->arc == aboveArc;
      };

      DeleteEventsWithPredicate(eventQueue, IsCircleEventOnAboveArc);

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

      gVisualizer->step();
      drawBeachLine(gVisualizer, rootArc, Pos.y, Yellow);
      drawHorizontalLine(gVisualizer, Pos, Red);
      drawEvents(gVisualizer, eventQueue);
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

  void draw(IDrawer* drawer) const override { drawer->rect(Pos - Vec2(0.2, 0.2), Vec2(0.4, 0.4), Green); }
};

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
      eventQueue.push_back(new siteEvent(point));
    }

    while(!eventQueue.empty())
    {
      auto compareEvents = [](Event* eventA, Event* eventB) { return eventA->pos().y < eventB->pos().y; };

      std::sort(eventQueue.begin(), eventQueue.end(), compareEvents);
      Event* event = eventQueue.back();
      eventQueue.pop_back();
      const Vec2 eventPos = event->pos();
      drawBeachLine(gVisualizer, rootArc, eventPos.y, Yellow);
      drawHorizontalLine(gVisualizer, eventPos, Red);
      drawEvents(gVisualizer, eventQueue);
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
