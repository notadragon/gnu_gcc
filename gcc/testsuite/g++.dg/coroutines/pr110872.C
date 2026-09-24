// This is now the run test its own NOTE asked to become; the ICE it was
// xfailed for is fixed.
//
// The predicate calls a member function on a class-typed result binding, so it
// needs that binding's address, and the binding had none -- hence
// expand_expr_addr_expr_1.  The coroutine is incidental: three lines with an
// ordinary function reproduce the same ICE,
//
//   struct G { bool is_valid () const { return false; } };
//   G make () post (g : g.is_valid ()) { return G{}; }
//   int main () { make (); }
//
// which makes this a duplicate of PR c++/125574, fixed by handing such a
// predicate an addressable temporary.
//
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for iostream" { ! hostedlib } }

#include <iostream>
#include <coroutine>


template <typename T>
struct generator
{
    struct promise_type
    {
        std::suspend_always yield_value(T) { return {}; }

        std::suspend_always initial_suspend() const noexcept { return {}; }
        std::suspend_never final_suspend() const noexcept { return {}; }
        void unhandled_exception() noexcept {}

        generator<T> get_return_object() noexcept { return {}; }
    };

    bool is_valid() const { return false; }
};

namespace std {
template <typename T, typename... Args>
struct coroutine_traits<generator<T>, Args...>
{
    using promise_type = typename generator<T>::promise_type;
};

};

generator<int> val(int v) 
  post (g: g.is_valid())
{
    std::cout << "coro initial" << std::endl;
    co_yield v;
    std::cout << "coro resumed" << std::endl;
}

int main() { 
    std::cout << "main initial" << std::endl;
    generator<int> s = val(1);
    (void)s;
    std::cout << "main continues" << std::endl;
}

// The line number is matched loosely so that editing the header above cannot
// silently turn this into a test of nothing.
// { dg-output "contract violation in function generator<int> val.int. at .*.C:\[0-9\]+: g.is_valid().*(\n|\r\n|\r)" }
