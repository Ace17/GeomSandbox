#pragma once

#include <memory>

#include "app.h"
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

struct AbstractAlgorithm
{
  virtual ~AbstractAlgorithm() = default;
  virtual void display() = 0;
  virtual void init() = 0;
  virtual void execute() = 0;
};

template<typename AlgoDef>
struct ConcreteAlgorithm : public AbstractAlgorithm
{
  using Traits = FuncTraits<decltype(AlgoDef::execute)>;
  using InputType = typename Traits::InputType;
  using OutputType = typename Traits::OutputType;

  InputType m_input;
  OutputType m_output;

  void display() override { AlgoDef::display(m_input, m_output); }
  void init() override { m_input = AlgoDef::generateInput(); }
  void execute() override { m_output = AlgoDef::execute(m_input); }
};

IApp* createAlgorithmApp(std::unique_ptr<AbstractAlgorithm> algo);
