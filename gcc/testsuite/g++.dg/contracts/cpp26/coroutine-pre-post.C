// Contracts on coroutine functions
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcoroutines" }

#include <coroutine>

struct task {
  struct promise_type {
    task get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };
};

struct value_task {
  struct promise_type {
    int result;
    value_task get_return_object() { return {this}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_value(int v) { result = v; }
    void unhandled_exception() {}
  };
  promise_type *p;
  int get() { return p->result; }
};

// Precondition on a void coroutine
task void_coro(int x)
  pre (x > 0)
{
  co_return;
}

// Precondition on a value-returning coroutine
// (postconditions on coroutines apply to the return object, not co_return value)
value_task value_coro(int x)
  pre (x > 0)
{
  co_return x * 2;
}

// Multiple contracts on a coroutine
task multi_contract(int a, int b)
  pre (a > 0)
  pre (b > 0)
  pre (a != b)
{
  co_return;
}

// Coroutine with awaitable
struct awaitable {
  bool await_ready() noexcept { return false; }
  void await_suspend(std::coroutine_handle<>) noexcept {}
  void await_resume() noexcept {}
};

task await_coro(int x)
  pre (x >= 0)
{
  co_await awaitable{};
  co_return;
}

// Template coroutine with contracts
template<typename T>
task tmpl_coro(T val)
  pre (val != T{})
{
  co_return;
}

void use() {
  void_coro(1);
  value_coro(5);
  multi_contract(1, 2);
  await_coro(0);
  tmpl_coro(42);
  tmpl_coro(3.14);
}
