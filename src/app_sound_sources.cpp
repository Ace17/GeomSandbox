// Copyright (C) 2024 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////

#include "core/app.h"
#include "core/drawer.h"
#include "core/geom.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <unordered_set>
#include <vector>

#include "random.h"

int64_t getSteadyClockUs()
{
  using namespace std::chrono;
  auto elapsedTime = steady_clock::now().time_since_epoch();
  return duration_cast<microseconds>(elapsedTime).count();
}

namespace
{

struct Circle
{
  Vec2 center;
  float radius;
};

int g_testCount;

bool IsPointInsideCircle(Vec2 p, Circle c)
{
  g_testCount++;

  auto delta = p - c.center;
  return dotProduct(delta, delta) < c.radius * c.radius;
}

[[maybe_unused]] void
IntersectLineSweep(const std::vector<Circle>& circles, const std::vector<Vec2>& points, std::vector<int>& result)
{
  g_testCount = 0;

  struct Event
  {
    float when;
    int type; // 0:enterCircle, 1:leaveCircle, 2:point
    int index; // circle or point
  };

  std::vector<Event> events;
  events.reserve(circles.size() * 2 + events.size());
  for(int i = 0; i < (int)circles.size(); ++i)
  {
    auto& c = circles[i];
    events.push_back({c.center.x - c.radius, 0, i});
    events.push_back({c.center.x + c.radius, 1, i});
  }

  for(int i = 0; i < (int)points.size(); ++i)
    events.push_back({points[i].x, 2, i});

  auto byWhen = [](const Event& a, const Event& b) { return a.when < b.when; };

  std::sort(events.begin(), events.end(), byWhen);

  std::unordered_set<int> currentCircles;
  currentCircles.reserve(circles.size() / 10);

  result.clear();
  result.resize(circles.size());

  for(auto& evt : events)
  {
    switch(evt.type)
    {
    case 0: // enterCircle
      currentCircles.insert(evt.index);
      break;
    case 1: // leaveCircle
      currentCircles.erase(evt.index);
      break;
    case 2: // point
    {
      auto it = currentCircles.begin();
      while(it != currentCircles.end())
      {
        const auto circleIndex = *it;
        if(IsPointInsideCircle(points[evt.index], circles[circleIndex]))
        {
          result[circleIndex] = 1;
          it = currentCircles.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }
    break;
    }
  }
}

[[maybe_unused]] void
IntersectQuadratic(const std::vector<Circle>& circles, const std::vector<Vec2>& points, std::vector<int>& result)
{
  g_testCount = 0;
  result.clear();
  result.resize(circles.size());

  for(int i = 0; i < (int)circles.size(); ++i)
  {
    for(auto& p : points)
    {
      if(IsPointInsideCircle(p, circles[i]))
        result[i] = 1;
    }
  }
}

struct AppSoundSources : IApp
{
  AppSoundSources()
  {
    m_sources.resize(randomInt(3, 10) * 2);
    for(auto& s : m_sources)
    {
      s.center = randomPos({-20, -10}, {20, 10});
      s.radius = randomFloat(0.3, 10.5);
    }

    m_probes.resize(randomInt(1, 4) * 2);
    for(auto& p : m_probes)
      p = randomPos({-20, -10}, {20, 10});

    m_sourceIsAudible.resize(m_sources.size());

    compute();
  }

  void draw(IDrawer* drawer) override
  {
    for(auto& s : m_sources)
    {
      const int i = int(&s - m_sources.data());
      auto color = m_sourceIsAudible[i] ? Yellow : Blue;
      // drawCross(drawer, s.center, color);
      drawer->circle(s.center, s.radius, color);
    }

    for(auto& p : m_probes)
      drawCross(drawer, p, Red);

    double ratio = g_testCount / double(m_sources.size() * m_probes.size());
    char buffer[256];
    sprintf(buffer, "Sources: %d, Probes: %d, Hit tests: %.2f%%\n", (int)m_sources.size(), (int)m_probes.size(), ratio);
    drawer->text({}, buffer, White);
  }

  void drawCross(IDrawer* drawer, Vec2 pos, Color color)
  {
    drawer->line(pos - Vec2{1, 0}, pos + Vec2{1, 0}, color);
    drawer->line(pos - Vec2{0, 1}, pos + Vec2{0, 1}, color);
  }

  void compute()
  {
    auto t0 = getSteadyClockUs();
    IntersectQuadratic(m_sources, m_probes, m_sourceIsAudible);
    auto t1 = getSteadyClockUs();
    for(int i = 0; i < 100; ++i)
      IntersectLineSweep(m_sources, m_probes, m_sourceIsAudible);
    auto t2 = getSteadyClockUs();
    fprintf(stderr, "%d sources, %d probes\n", (int)m_sources.size(), (int)m_probes.size());
    fprintf(stderr, "LineSweep Time: %.1f ms\n", (t2 - t1) / 1000.0 / 100.0);
    fprintf(stderr, "Quadratic Time: %.1f ms\n", (t1 - t0) / 1000.0);
  }

  std::vector<int> m_sourceIsAudible;
  std::vector<Circle> m_sources;
  std::vector<Vec2> m_probes;
};

IApp* create() { return new AppSoundSources; }
const int registered = registerApp("App.SoundSources", &create);
}
