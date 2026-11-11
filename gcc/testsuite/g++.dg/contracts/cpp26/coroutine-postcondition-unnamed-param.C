// A coroutine whose definition leaves unnamed a by-value parameter that a
// postcondition on another declaration odr-uses: not const, it gets both of
// coroutine-postcondition-param.C's diagnostics (the const rule and the
// coroutine rule); const, the coroutine rule alone.  Mirrored from the EDG
// front end, which missed them (EDG-107 in the edgcpp_compiler fork).
//
// Mirror: tests/tests/contracts/sema/postcondition_coroutine_unnamed.sft.cpp
// in the edgcpp_compiler fork;
// clang/test/Contracts/coroutine-postcondition-unnamed-param.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

#include <coroutine>

struct task
{
  struct promise_type
  {
    task get_return_object () { return {}; }
    std::suspend_never initial_suspend () { return {}; }
    std::suspend_never final_suspend () noexcept { return {}; }
    void return_void () {}
    void unhandled_exception () {}
  };
};

bool flag = true;

task c1 (const int b) post (b > 0);
task c1 (int) { co_return; }	// { dg-error "value parameter (.<anonymous>.|.b.) used in a postcondition must be const" }
// { dg-error "odr-used in a postcondition of a coroutine" "" { target *-*-* } .-1 }
task c2 (const int b) post (b > 0);
task c2 (const int) { co_return; }	// { dg-error "odr-used in a postcondition of a coroutine" }
task c3 (const int b) post (flag);
task c3 (int) { co_return; }	// OK
