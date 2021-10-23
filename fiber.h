#pragma once
#include <cstdint>
#include <vector>

class Fiber
{
public:
  Fiber(void(*func)(void*), void* userParam);

  void resume();
  bool finished() const { return m_finished; };
  static void yield();

private:
  static void launcherFunc();

  // Function to run inside the fiber
  void(*const m_func)(void*);
  void* const m_userParam;

  // Execution state
  std::vector<uint8_t> m_stack;
  bool m_finished = false;

  // OS-specific metadata
  struct Priv;
  Priv* priv;
  alignas(void*) uint8_t privBuffer[2048];
};

