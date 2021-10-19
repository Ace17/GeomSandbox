#pragma once
#include <cstdint>
#include <vector>

class Fiber
{
public:
  Fiber(void(*func)());

  void resume();
  static void yield();

private:
  struct Priv;

  alignas(void*) uint8_t privBuffer[2048];
  Priv* priv;

  std::vector<uint8_t> stack;
};

