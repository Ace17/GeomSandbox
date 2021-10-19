#include "fiber.h"
#include <ucontext.h>

void makecontext(ucontext_t* ucp, void (* func)(), int argc, ...);
int swapcontext(ucontext_t* oucp, const ucontext_t* ucp);

static thread_local Fiber* ThisFiber;

struct Fiber::Priv
{
  ucontext_t main;
  ucontext_t client;
};

static_assert(sizeof(Fiber::Priv) < sizeof(Fiber::privBuffer));

Fiber::Fiber(void(*func)())
{
  stack.resize(1024 * 1024);
  priv = new(privBuffer) Fiber::Priv;

  getcontext(&priv->client);
  priv->client.uc_stack.ss_sp = stack.data();
  priv->client.uc_stack.ss_size = stack.size();

  makecontext(&priv->client, func, 0);
}

void Fiber::resume()
{
  ThisFiber = this;
  swapcontext(&priv->main, &priv->client);
}

void Fiber::yield()
{
  swapcontext(&ThisFiber->priv->client, &ThisFiber->priv->main);
}

