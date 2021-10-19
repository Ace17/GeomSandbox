#pragma once
#include <cstdint>
#include <vector>

class Fiber
{
public:
  Fiber(void(*func)());

  void resume();
  static void yield();

  struct Priv;

  alignas(void*) uint8_t privBuffer[2048];
  Priv* priv;

private:
  std::vector<uint8_t> stack;
};

