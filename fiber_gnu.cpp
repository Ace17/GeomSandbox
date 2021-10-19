#include "fiber.h"
#include <cassert>
#include <ucontext.h>

static thread_local Fiber* ThisFiber;

struct Fiber::Priv
{
  ucontext_t main;
  ucontext_t client;
};

void Fiber::launcherFunc()
{
  ThisFiber->m_func();
  ThisFiber->m_finished = true;
  yield();
}

Fiber::Fiber(void(*func)()) : m_func(func)
{
  static_assert(sizeof(Fiber::Priv) < sizeof(Fiber::privBuffer));

  m_stack.resize(1024 * 1024);
  priv = new(privBuffer) Fiber::Priv;

  getcontext(&priv->client);
  priv->client.uc_stack.ss_sp = m_stack.data();
  priv->client.uc_stack.ss_size = m_stack.size();

  makecontext(&priv->client, launcherFunc, 0);
}

void Fiber::resume()
{
  if(m_finished)
    return;

  assert(ThisFiber == nullptr);
  ThisFiber = this;
  swapcontext(&priv->main, &priv->client);
}

void Fiber::yield()
{
  auto pThis = ThisFiber;
  ThisFiber = nullptr;
  swapcontext(&pThis->priv->client, &pThis->priv->main);
}

