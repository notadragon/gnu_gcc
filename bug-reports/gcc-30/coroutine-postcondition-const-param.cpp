// An odr-use of a non-reference parameter in a coroutine's postcondition is
// accepted when the parameter is written const.
//
//   g++ -std=c++26 -fsyntax-only \
//       coroutine-postcondition-const-param.cpp
//
//     -> by_value is accepted; it is ill-formed.
//
// [dcl.fct.def.coroutine] says so in a note outright:
//
//   "An odr-use of a non-reference parameter in a postcondition assertion of
//    a coroutine is ill-formed."

#include <coroutine>

struct Task {
    struct promise_type {
        Task get_return_object() { return {}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() { }
        void unhandled_exception() { }
    };
};

// THE BUG: accepted; ill-formed.
Task by_value(const int x) post(x > 0) { co_return; }

// CONTROL: the non-const spelling is caught, by the ordinary const rule
// rather than by anything coroutine-aware.  That is what says the coroutine
// restriction itself is missing rather than merely mis-worded.
Task non_const(int x) post(x > 0) { co_return; }

// CONTROL: a reference parameter is fine -- [dcl.fct.def.coroutine]/5 binds
// its frame copy to the same object -- and must keep compiling.
Task by_ref(const int& x) post(x > 0) { co_return; }

// CONTROL: a precondition may name a by-value parameter; the const rule, and
// so this restriction, are postcondition rules.
Task in_pre(int x) pre(x > 0) { co_return; }

// CONTROL: a non-coroutine with the same signature is unaffected.
int not_a_coroutine(const int x) post(x > 0) { return x; }
