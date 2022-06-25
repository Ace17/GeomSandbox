#pragma once

#include <cassert>
#include <memory>
#include <string>

#include "app.h"
#include "drawer.h"
#include "fiber.h"
#include "sandbox.h"
// Example algorithm :
// struct MyAlgorithm
// {
// static std::vector<Vec2> generateInput();
// static std::vector<Edge> execute(std::vector<Vec2> input);
// static void drawInput(const std::vector<Vec2>& input);
// static void drawOutput(const std::vector<Edge>& output);
// };

template<typename>
struct FuncTraits;

template<typename ReturnTypeP, typename ArgTypeP>
struct FuncTraits<ReturnTypeP(ArgTypeP)>
{
  using OutputType = ReturnTypeP;
  using InputType = ArgTypeP;
};

template<typename Algorithm>
struct AlgorithmApp : IApp
{
  using Traits = FuncTraits<decltype(Algorithm::execute)>;
  using InputType = typename Traits::InputType;
  using OutputType = typename Traits::OutputType;

  InputType m_input;
  OutputType m_output;

  AlgorithmApp() { m_input = Algorithm::generateInput(); }

  void draw(IDrawer* drawer) override
  {
    m_visuForFrame = {};

    gVisualizer = &m_visuForFrame;
    Algorithm::drawInput(m_input);
    Algorithm::drawOutput(m_input, m_output);
    gVisualizer = gNullVisualizer;

    m_visuForFrame.m_frontScreen = std::move(m_visuForFrame.m_screen);

    m_visuForFrame.flush(drawer);
    m_visuForAlgo.flush(drawer);
  }

  static void staticExecute(void* userParam) { ((AlgorithmApp<Algorithm>*)userParam)->executeFromFiber(); }

  void executeFromFiber()
  {
    // clear visualization
    m_visuForAlgo.m_screen = {};
    m_visuForAlgo.m_insideAlgorithmExecute = true;

    m_output = Algorithm::execute(m_input);

    // clear last step's visualisation
    m_visuForAlgo.m_insideAlgorithmExecute = false;
    m_visuForAlgo.m_frontScreen = {};
  }

  void keydown(Key key) override
  {
    if(key == Key::Space || key == Key::Return)
    {
      gVisualizer = &m_visuForAlgo;

      if(!m_fiber)
        m_fiber = std::make_unique<Fiber>(staticExecute, this);

      if(key == Key::Return)
      {
        while(!m_fiber->finished())
          m_fiber->resume();
      }
      else
        m_fiber->resume();

      gVisualizer = gNullVisualizer;
    }
  }

  std::unique_ptr<Fiber> m_fiber;

  struct Visualizer : IVisualizer
  {
    struct VisualLine
    {
      Vec2 a, b;
      Color color;
    };

    struct VisualRect
    {
      Vec2 a, b;
      Color color;
    };

    struct VisualCircle
    {
      Vec2 center;
      float radius;
      Color color;
    };

    struct VisualText
    {
      Vec2 pos;
      std::string text;
      Color color;
    };

    struct ScreenState
    {
      std::vector<VisualLine> lines;
      std::vector<VisualRect> rects;
      std::vector<VisualCircle> circles;
      std::vector<VisualText> texts;
    };

    bool m_insideAlgorithmExecute = false;
    ScreenState m_screen; // the frame we're building
    ScreenState m_frontScreen; // the frame we're currently showing

    void rect(Vec2 a, Vec2 b, Color color) { m_screen.rects.push_back({a, b, color}); }
    void circle(Vec2 center, float radius, Color color) { m_screen.circles.push_back({center, radius, color}); }
    void text(Vec2 pos, const char* text, Color color) { m_screen.texts.push_back({pos, text, color}); }
    void line(Vec2 a, Vec2 b, Color c) override { m_screen.lines.push_back({a, b, c}); }

    void step() override
    {
      assert(m_insideAlgorithmExecute);
      m_frontScreen = std::move(m_screen);
      Fiber::yield();
    }

    void flush(IDrawer* drawer)
    {
      for(auto& line : m_frontScreen.lines)
        drawer->line(line.a, line.b, line.color);

      for(auto& rect : m_frontScreen.rects)
        drawer->rect(rect.a, rect.b, rect.color);

      for(auto& circle : m_frontScreen.circles)
        drawer->circle(circle.center, circle.radius, circle.color);

      for(auto& text : m_frontScreen.texts)
        drawer->text(text.pos, text.text.c_str(), text.color);
    }
  };

  Visualizer m_visuForAlgo;
  Visualizer m_visuForFrame;
};
