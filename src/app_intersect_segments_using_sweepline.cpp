// Copyright (C) 2025 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// intersection detection in segment soup

#include "core/algorithm_app.h"
#include "core/geom.h"
#include "core/sandbox.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <set>
#include <vector>

#include "random.h"

bool segmentsIntersect(Vec2 u0, Vec2 u1, Vec2 v0, Vec2 v1, Vec2& where);

namespace
{
Vec2 randomPosition()
{
  Vec2 r;
  r.x = randomFloat(-30, 30);
  r.y = randomFloat(-15, 15);
  return r;
}

Vec2 randomDelta()
{
  Vec2 r;
  r.x = randomFloat(-7, +7);
  r.y = randomFloat(-7, +7);
  return r;
}

struct SegmentIntersectionUsingSweepline
{
  static std::vector<Vec2> generateInput()
  {
    const int N = 200;

    std::vector<Vec2> points;

    for(int i=0;i < N;++i)
    {
      auto a = randomPosition();
      auto b = a + randomDelta();
      points.push_back(a);
      points.push_back(b);
    }

    return points;
  }

  static void display(span<const Vec2> points, span<const Vec2> intersections)
  {
    for(int i = 0; i + 1 < (int)points.len; i += 2)
      sandbox_line(points[i], points[i + 1], White);

    for(auto intersection : intersections)
    {
      sandbox_line(intersection - Vec2(0.4, 0), intersection + Vec2(0.4, 0), Green);
      sandbox_line(intersection - Vec2(0, 0.4), intersection + Vec2(0, 0.4), Green);
    }
  }

  static std::vector<Vec2> execute(std::vector<Vec2> points)
  {
    struct Event
    {
      float t;
      float t2;
      int type; // 0:begin, 1:end
      int segment; // index of the first vertex in 'points'
    };

    auto byTime = [&](const Event& a, const Event& b)
    {
      if(a.t != b.t)
        return a.t > b.t;
      if(a.t2 != b.t2)
        return a.t2 > b.t2;
      return false;
    };

    std::vector<Event> events;

    for(int i = 0; i + 1 < (int)points.size(); i += 2)
    {
      Event a{points[i + 0].y, points[i + 0].x, -1, i};
      Event b{points[i + 1].y, points[i + 1].x, -1, i};

      if(!byTime(a, b))
        std::swap(a, b);

      a.type = 0;
      b.type = 1;
      events.push_back(a);
      events.push_back(b);
    }

    std::sort(events.begin(), events.end(), byTime);

    {
      for(auto& evt : events)
        sandbox_line({-100, evt.t}, {+100, evt.t}, Gray);
      sandbox_breakpoint();
    }

    std::vector<Vec2> result;

    std::set<int> currSegments;
    for(auto& evt : events)
    {
      if(evt.type == 0)
      {
        for(auto& other : currSegments)
        {
          const auto u0 = points[evt.segment + 0];
          const auto u1 = points[evt.segment + 1];
          const auto v0 = points[other + 0];
          const auto v1 = points[other + 1];
          Vec2 where;
          if(segmentsIntersect(u0, u1, v0, v1, where))
            result.push_back(where);
        }
        currSegments.insert(evt.segment);
      }
      else
      {
        currSegments.erase(evt.segment);
      }

      {
        sandbox_line({-100, evt.t}, {+100, evt.t}, Yellow);
        for(auto seg : currSegments)
          sandbox_line(points[seg], points[seg + 1], Green);

        for(auto intersection : result)
        {
          sandbox_line(intersection - Vec2(0.4, 0), intersection + Vec2(0.4, 0), Yellow);
          sandbox_line(intersection - Vec2(0, 0.4), intersection + Vec2(0, 0.4), Yellow);
        }
        sandbox_breakpoint();
      }
    }

    return result;
  }
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<SegmentIntersectionUsingSweepline>>()); }
const int registered = registerApp("Intersection/SegmentsUsingSweepline", &create);
}
