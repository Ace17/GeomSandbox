#include <cassert>
#include <windows.h>
#include <winnt.h>

#include "core/fiber.h"

static thread_local Fiber* ThisFiber;

struct Fiber::Priv
{
  CONTEXT client{};
  CONTEXT main{};
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

  m_stack.resize(1024 * 1024);
  priv = new(privBuffer) Fiber::Priv;

  priv->client.ContextFlags = CONTEXT_FULL;
  RtlCaptureContext(&priv->client);

  uint64_t rsp = (uintptr_t)m_stack.back();

  rsp = rsp - rsp % 8;

  priv->client.Rip = (uintptr_t)Fiber::launcherFunc;
  priv->client.Rsp = rsp;
}

void Fiber::resume()
{
  if(m_finished)
    return;

  assert(ThisFiber == nullptr);
  ThisFiber = this;

  RtlCaptureContext(&priv->main);
  RtlRestoreContext(&priv->client, nullptr);
}

void Fiber::yield()
{
  auto pThis = ThisFiber;
  ThisFiber = nullptr;
  RtlCaptureContext(&pThis->priv->client);
  RtlRestoreContext(&pThis->priv->main, nullptr);
}
