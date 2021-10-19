#include "fiber.h"
#include <cassert>
#include <ucontext.h>

void makecontext(ucontext_t* ucp, void (* func)(), int argc, ...);
int swapcontext(ucontext_t* oucp, const ucontext_t* ucp);

static thread_local Fiber* ThisFiber;

struct Fiber::Priv
{
  ucontext_t main;
  ucontext_t client;
};

Fiber::Fiber(void(*func)())
{
  static_assert(sizeof(Fiber::Priv) < sizeof(Fiber::privBuffer));

  stack.resize(1024 * 1024);
  priv = new(privBuffer) Fiber::Priv;

  getcontext(&priv->client);
  priv->client.uc_stack.ss_sp = stack.data();
  priv->client.uc_stack.ss_size = stack.size();

  makecontext(&priv->client, func, 0);
}

void Fiber::resume()
{
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

