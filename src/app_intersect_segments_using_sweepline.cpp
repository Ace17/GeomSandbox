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
#include <cassert>
#include <cmath>
#include <memory>
#include <set>
#include <vector>

#include "random.h"

bool segmentsIntersect(Vec2 u0, Vec2 u1, Vec2 v0, Vec2 v1, Vec2& where);

namespace
{
int queryCount;

Vec2 randomPosition()
{
  Vec2 r;
  r.x = randomFloat(-20, 20);
  r.y = randomFloat(-15, 15);
  return r;
}

[[maybe_unused]] Vec2 randomDelta()
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
    const int N = 20; // 200;

    std::vector<Vec2> points;

    for(int i = 0; i < N; ++i)
    {
      auto a = randomPosition();
      auto b = randomPosition(); // a + randomDelta();
      a.x = floor(a.x);
      b.x = floor(b.x);
      a.y = floor(a.y);
      b.y = floor(b.y);
      points.push_back(a);
      points.push_back(b);
    }

    return points;
  }

  static void display(span<const Vec2> points, span<const Vec2> intersections)
  {
    for(int i = 0; i + 1 < (int)points.len; i += 2)
    {
      sandbox_line(points[i], points[i + 1], White);

      {
        auto a = points[i];
        auto b = points[i + 1];
        if(comparePoints(a, b) == 1)
          std::swap(a, b);
        char buf[256];
        sprintf(buf, "%d", i);
        sandbox_text(b, buf, White, {-48, 0});
      }
    }

    for(auto intersection : intersections)
    {
      sandbox_line(intersection - Vec2(0.4, 0), intersection + Vec2(0.4, 0), Green);
      sandbox_line(intersection - Vec2(0, 0.4), intersection + Vec2(0, 0.4), Green);
    }

    {
      char msg[256];
      sprintf(msg, "%d intersection tests\n", queryCount);
      sandbox_text({}, msg);
    }
  }

  static int comparePoints(Vec2 a, Vec2 b)
  {
    if(a.y != b.y)
      return a.y > b.y ? -1 : +1;
    if(a.x != b.x)
      return a.x < b.x ? -1 : +1;

    return 0;
  }

  static std::vector<Vec2> execute(std::vector<Vec2> points)
  {
    queryCount = 0;

    enum EventType
    {
      Begin,
      End,
      Cross,
    };

    struct Event
    {
      EventType type;
      Vec2 pos;
      int segment; // index of the first vertex in 'points'
    };

    // order the endpoints, the upper one goes first.
    for(int i = 0; i + 1 < (int)points.size(); i += 2)
    {
      if(comparePoints(points[i], points[i + 1]) == 1)
        std::swap(points[i], points[i + 1]);
    }

    static auto EventsByTime = [](const Event& a, const Event& b)
    {
      if(a.pos.y != b.pos.y)
        return a.pos.y > b.pos.y;
      if(a.pos.x != b.pos.x)
        return a.pos.x < b.pos.x;
      return a.type < b.type;
    };

    std::set<Event, decltype(EventsByTime)> events(EventsByTime);

    for(int i = 0; i + 1 < (int)points.size(); i += 2)
    {
      events.insert({Begin, points[i + 0], i});
      events.insert({End, points[i + 1], i});
    }

    {
      for(auto& evt : events)
        sandbox_line({-100, evt.pos.y}, {+100, evt.pos.y}, Gray);
      sandbox_breakpoint();
    }

    std::vector<Vec2> result;

    struct ActiveSegment
    {
      int segment; // index of the first vertex in 'points'
    };

    static auto firstVertex = [](int vertexId)
    {
      if(vertexId % 2 == 0)
        return vertexId;
      else
        return vertexId - 1;
    };

    static auto secondVertex = [](int vertexId)
    {
      if(vertexId % 2 == 0)
        return vertexId + 1;
      else
        return vertexId;
    };

    auto getSegmentX = [&](int segment, float y)
    {
      auto a = points[segment + 0];
      auto b = points[segment + 1];
      if(a.x == b.x || a.y == b.y)
        return a.x;

      return (y - a.y) / (b.y - a.y) * (b.x - a.x) + a.x;
    };

    float currY = 0;
    auto ActiveSegmentsByX = [&currY, &getSegmentX](const ActiveSegment& a, const ActiveSegment& b)
    {
      float xA = getSegmentX(firstVertex(a.segment), currY);
      float xB = getSegmentX(firstVertex(b.segment), currY);
      return xA < xB;
    };

    std::multiset<ActiveSegment, decltype(ActiveSegmentsByX)> activeSegments(ActiveSegmentsByX);

    auto checkForIntersections = [&points,&result,&events](int segLeft, int segRight, float minY)
    {
      printf("check for intersection between %d and %d\n", segLeft, segRight);
      // be invariant to who is u and who is v: in order to properly deduplicate
      // intersection events, we must always compute exactly the same intersection
      // point coordinates.
      const int segMin = std::min(segLeft, segRight);
      const int segMax = std::max(segLeft, segRight);
      const auto u0 = points[firstVertex(segMin)];
      const auto u1 = points[secondVertex(segMin)];
      const auto v0 = points[firstVertex(segMax)];
      const auto v1 = points[secondVertex(segMax)];
      Vec2 where;
      queryCount++;
      if(!segmentsIntersect(u0, u1, v0, v1, where))
        return;

      if(where.y >= minY)
        return;

      result.push_back(where);

      auto [it, inserted] = events.insert({Cross, where, segLeft});

      char buf[256];
      sprintf(buf, "push intersection between (%d,%d) at %.1f,%.1f%s", segLeft, segRight, where.x, where.y,
            inserted ? "" : " (already present)");
      sandbox_text(where, buf, Red, {8, 0});
      sandbox_circle(where, 0, Red, 4);
    };

    while(events.size())
    {
      const Event evt = *events.begin();
      events.erase(events.begin());

      if(evt.type == Begin) // one segment starts
      {
        currY = evt.pos.y;
        auto it = activeSegments.insert({evt.segment});

        // check for left neighbor
        if(it != activeSegments.begin())
        {
          auto left = it;
          --left;

          checkForIntersections(left->segment, evt.segment, evt.pos.y);
        }

        // check for right neighbor
        auto right = it;
        ++right;
        if(right != activeSegments.end())
        {
          auto other = right->segment;

          checkForIntersections(evt.segment, other, evt.pos.y);
        }
      }
      else if(evt.type == End) // one segment ends
      {
        currY = evt.pos.y;
        ActiveSegment key{evt.segment};
        auto it = activeSegments.find(key);
        if(it == activeSegments.end())
        {
          fprintf(stderr, "Can't remove %d in: \n", evt.segment);

          for(auto it : activeSegments)
          {
            fprintf(stderr, "%d (%.2f)\n", it.segment, getSegmentX(it.segment, currY));
          }
        }
        assert(it != activeSegments.end());
        it = activeSegments.erase(it);
        fprintf(stderr, "removed %d\n", evt.segment);

        if(it != activeSegments.end() && it != activeSegments.begin())
        {
          auto prev = it;
          prev--;
          checkForIntersections(prev->segment, it->segment, evt.pos.y);
        }
      }
      else // two segments cross: swap them in 'ActiveSegments'
      {
        // temporarily bias currY, so the old left and old right segments can
        // be determined.
        // (as we're on a crossing point, without the bias, they have the same key)
        auto prevCurrY = currY;
        currY = (evt.pos.y + prevCurrY) * 0.5;

        auto leftSegment = evt.segment;
        ActiveSegment key = {leftSegment};
        auto it = activeSegments.find(key);
        if(it == activeSegments.end())
        {
          fprintf(stderr, "Can't find %d in: \n", leftSegment);

          for(auto it : activeSegments)
          {
            fprintf(stderr, "%d (%.2f)\n", it.segment, getSegmentX(it.segment, currY));
          }
        }
        assert(it != activeSegments.end());
        auto it2 = activeSegments.erase(it);
        assert(it2 != activeSegments.end());
        auto rightSegment = it2->segment;
        activeSegments.erase(it2);

        printf("Removed left (%d) and right (%d) segments (%d active)\n", leftSegment, rightSegment,
              (int)activeSegments.size());

        assert(leftSegment != rightSegment);

        // temporarily bias currY, so the insertion into the std::set doesn't merge both segments
        // (as we're on a crossing point, without the bias, they have the same key)
        currY = evt.pos.y - 0.00001;

        // reinsert both active segments with updated x values
        ActiveSegment leftActiveSegment = {leftSegment};
        auto rightIt = activeSegments.insert(leftActiveSegment);
        printf("Reinserted (old left, as new right) %d (%d active)\n", leftSegment, (int)activeSegments.size());
        {
          rightIt++;
          if(rightIt != activeSegments.end())
          {
            checkForIntersections(leftSegment, rightIt->segment, evt.pos.y);
          }
        }

        ActiveSegment rightActiveSegment = {rightSegment};
        auto leftIt = activeSegments.insert(rightActiveSegment);
        printf("Reinserted (old right, as new left) %d (%d active)\n", rightSegment, (int)activeSegments.size());
        {
          if(leftIt != activeSegments.begin())
          {
            leftIt--;
            checkForIntersections(leftIt->segment, rightSegment, evt.pos.y);
          }
        }

        currY = evt.pos.y;
      }

      // draw
      {
        {
          const char* msg;
          if(evt.type == 0)
            msg = "begin";
          else if(evt.type == 1)
            msg = "end";
          else
            msg = "cross";
          sandbox_text({-20, evt.pos.y}, msg, Yellow, {0, -8});
          sandbox_line({-100, evt.pos.y}, {+100, evt.pos.y}, Yellow);
        }

        int idx = 0;
        for(auto& activeSeg : activeSegments)
        {
          sandbox_line(points[firstVertex(activeSeg.segment)], points[secondVertex(activeSeg.segment)], Green);
          char buf[256];
          sprintf(buf, "%d", idx++);
          sandbox_text(points[activeSeg.segment], buf, Green, {8, 8});
          sandbox_circle({getSegmentX(activeSeg.segment, evt.pos.y), evt.pos.y}, 0, Yellow, 8);
        }

        for(auto intersection : result)
        {
          sandbox_line(intersection - Vec2(0.4, 0), intersection + Vec2(0.4, 0), Yellow);
          sandbox_line(intersection - Vec2(0, 0.4), intersection + Vec2(0, 0.4), Yellow);
        }

        {
          for(auto& evt : events)
          {
            sandbox_line({-100, evt.pos.y}, {+100, evt.pos.y}, Gray);
            const char* msg;
            if(evt.type == 0)
              msg = "begin";
            else if(evt.type == 1)
              msg = "end";
            else
              msg = "cross";
            sandbox_text({-20, evt.pos.y}, msg, Gray, {0, -8});
          }
        }

        sandbox_breakpoint();
      }
    }

    return result;
  }

  inline static const TestCase<std::vector<Vec2>> AllTestCases[] =
  {
    {
      "Basic 1",
      {
        {-10, -8},
        {+10, +12},

        {+10, -8},
        {-10, +12},

        {+3, -11},
        {-13, +9},
      },
    },

    {
      "Basic 2",
      {
        {-20, -1},
        {+20, -7},

        {-10, -14},
        {1, +10},

        {-5, +13},
        {+13, -7},
      },
    },

    {
      "T-junction on start",
      {
        {-10,12},
        {+10,-8},

        {-10, -10},
        {0, 2},
      },
    },

    {
      "T-junction on end",
      {
        {-10,-12},
        {+10,+8},

        {-10, +10},
        {0, -2},
      },
    },
  };
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<SegmentIntersectionUsingSweepline>>()); }
const int registered = registerApp("Intersection/SegmentsUsingSweepline", &create);
}
