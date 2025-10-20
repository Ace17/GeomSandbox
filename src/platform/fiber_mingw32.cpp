#include "core/fiber.h"

#include <cassert>
#include <windows.h>

static thread_local Fiber* ThisFiber;

struct Fiber::Priv
{
  void* main;
  void* fiber;
};

void Fiber::launcherFunc()
{
  ThisFiber->m_func(ThisFiber->m_userParam);
  ThisFiber->m_finished = true;
  yield();
}

Fiber::Fiber(void (*func)(void*), void* userParam)
    : m_func(func)
    , m_userParam(userParam)
{
  static_assert(sizeof(Fiber::Priv) < sizeof(Fiber::privBuffer));

  priv = new(privBuffer) Fiber::Priv;

  static auto wrapper = [](void*) { launcherFunc(); };
  priv->main = ConvertThreadToFiber(nullptr);
  priv->fiber = CreateFiber(1024 * 1024, wrapper, nullptr);
}

void Fiber::resume()
{
  if(m_finished)
    return;

  assert(ThisFiber == nullptr);
  ThisFiber = this;

  SwitchToFiber(priv->fiber);
}

void Fiber::yield()
{
  auto pThis = ThisFiber;
  ThisFiber = nullptr;
  SwitchToFiber(pThis->priv->main);
}
