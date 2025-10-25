#pragma once

#include <any>
#include <stdint.h>
#include <vector>

#include "app.h"

template<typename InputType, typename OutputType = int>
struct TestCase
{
  const char* name;
  InputType inputData;
  void (*checkOutput)(const OutputType&) = nullptr;
};

struct AlgorithmDescription
{
  const char* name;

  std::any (*generateInputFunc)(int) = nullptr;
  std::any (*executeFunc)(std::any) = nullptr;
  std::any (*getTestCaseInput)(int) = nullptr;
  void (*checkTestCaseOutput)(int, const std::any*) = nullptr;
  std::any (*loadInputFunc)(span<const uint8_t>) = nullptr;
  void (*displayFunc)(const std::any*, const std::any*) = nullptr;

  std::vector<const char*> testCases;
};

template<typename>
struct FuncTraits;

template<typename ReturnTypeP, typename ArgTypeP>
struct FuncTraits<ReturnTypeP(ArgTypeP)>
{
  using OutputType = typename std::remove_reference<ReturnTypeP>::type;
  using InputType = typename std::remove_reference<ArgTypeP>::type;
};

// clang-format off
#define BEGIN_ALGO(name_, algoFunc) \
static int reg = [](){ \
  using Traits = FuncTraits<decltype(algoFunc)>; \
  using InputType = typename Traits::InputType; \
  using OutputType [[maybe_unused]] = typename Traits::OutputType; \
  static AlgorithmDescription algoDesc; \
  algoDesc.name = name_; \
  algoDesc.executeFunc = [](std::any input) { return std::any(algoFunc(std::any_cast<InputType>(input))); };

#define END_ALGO \
  registerApp(algoDesc.name, [](){ return createAlgorithmApp2(algoDesc); }); \
  return 0; }();

#define WITH_INPUTGEN(func) \
  using GenTraits = FuncTraits<decltype(func)>; \
  static_assert(std::is_same_v<GenTraits::OutputType, InputType>); \
  algoDesc.generateInputFunc = [](int seed) { return std::any(func(seed)); };

#define WITH_TESTCASES(testCases_) \
  static_assert(std::is_same_v<decltype(testCases_[0].inputData), InputType>); \
  for(auto& tc : testCases_) \
    algoDesc.testCases.push_back(tc.name); \
  algoDesc.getTestCaseInput = [](int i) { return std::any(testCases_[i].inputData); }; \
  algoDesc.checkTestCaseOutput = [](int i, const std::any* output) { testCases_[i].checkOutput(*std::any_cast<OutputType>(output)); };

#define WITH_DISPLAY(func) \
  algoDesc.displayFunc = [](const std::any* input, const std::any* output) \
  {  \
    const InputType& in = *std::any_cast<InputType>(input); \
    const OutputType& out = output->has_value() ? *std::any_cast<OutputType>(output) : OutputType(); \
    func(in, out);\
  };

// clang-format on
IApp* createAlgorithmApp2(const AlgorithmDescription& desc);
