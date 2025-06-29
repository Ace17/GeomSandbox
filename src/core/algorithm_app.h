#pragma once

#include <cassert>
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

template<typename T>
struct RemoveRef
{
  using type = T;
};

template<typename T>
struct RemoveRef<const T&>
{
  using type = T;
};

template<typename ReturnTypeP, typename ArgTypeP>
struct FuncTraits<ReturnTypeP(ArgTypeP)>
{
  using OutputType = typename RemoveRef<ReturnTypeP>::type;
  using InputType = typename RemoveRef<ArgTypeP>::type;
};

struct AbstractAlgorithm
{
  virtual ~AbstractAlgorithm() = default;
  virtual void display() = 0;
  virtual void init() = 0;
  virtual void execute() = 0;
  virtual void loadInput(span<const uint8_t> data) = 0;
};

template<typename T>
T deserialize(span<const uint8_t> /*data*/)
{
  assert(false && "not implemented for this algorithm");
}

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
  void loadInput(span<const uint8_t> data) override { m_input = deserialize<InputType>(data); }
};

IApp* createAlgorithmApp(std::unique_ptr<AbstractAlgorithm> algo);
