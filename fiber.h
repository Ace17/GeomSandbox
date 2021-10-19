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
  bool finished = false;

  static void launcherFunc();
  struct Priv;

  alignas(void*) uint8_t privBuffer[2048];
  Priv* priv;

  void (* m_func)();
  std::vector<uint8_t> stack;
};

