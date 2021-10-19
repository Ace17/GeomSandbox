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
  static void launcherFunc();

  // Function to run inside the fiber
  void (* m_func)();

  // Execution state
  std::vector<uint8_t> m_stack;
  bool m_finished = false;

  // OS-specific metadata
  struct Priv;
  Priv* priv;
  alignas(void*) uint8_t privBuffer[2048];
};

