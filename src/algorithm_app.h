#pragma once

#include <memory>

#include "app.h"
#include "fiber.h"
#include "visualizer.h"
// Example algorithm :
// struct MyAlgorithm
// {
//   static std::vector<Vec2> generateInput();
//   static std::vector<Edge> execute(std::vector<Vec2> input);
//   static void drawInput(IDrawer* drawer, const std::vector<Vec2>& input);
//   static void drawOutput(IDrawer* drawer, const std::vector<Edge>& output);
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

  AlgorithmApp()
  {
    m_input = Algorithm::generateInput();
  }

  void draw(IDrawer* drawer) override
  {
    Algorithm::drawInput(drawer, m_input);

    for(auto& line : m_visu.m_lines)
      drawer->line(line.a, line.b, Yellow);

    Algorithm::drawOutput(drawer, m_input, m_output);
  }

  static void staticExecute(void* userParam)
  {
    ((AlgorithmApp<Algorithm>*)userParam)->executeFromFiber();
  }

  void executeFromFiber()
  {
    m_output = Algorithm::execute(m_input);
  }

  void keydown(Key key) override
  {
    if(key == Key::Space || key == Key::Return)
    {
      gVisualizer = &m_visu;

      if(!m_fiber)
        m_fiber = std::make_unique<Fiber>(staticExecute, this);

      if(key == Key::Return)
      {
        while(!m_fiber->finished())
          m_fiber->resume();
      }
      else
        m_fiber->resume();
    }
  }

  std::unique_ptr<Fiber> m_fiber;

  struct Visualizer : IVisualizer
  {
    void line(Vec2 a, Vec2 b) override { m_lines.push_back({ a, b }); }
    void step() override { Fiber::yield(); m_lines.clear();}

    struct VisualLine
    {
      Vec2 a, b;
    };

    std::vector<VisualLine> m_lines;
  };

  Visualizer m_visu;
};
