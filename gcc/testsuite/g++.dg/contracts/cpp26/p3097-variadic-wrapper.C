// The P3097 virtual-call wrapper of a C-variadic virtual function passes the
// variadic arguments on to the final overrider with __builtin_va_arg_pack
// (DECISIONS.md K12).  The caller-side wrapper does the same
// (caller-side-variadic.C).
//
// Mirror: clang/test/Contracts/p3097-variadic-wrapper.cpp in the
// llvm_llvm-project fork, where the call is checked at the call site.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cstdarg>

struct B {
  virtual int sum (int n, ...) pre (n >= 0);
};

int B::sum (int n, ...)
{
  va_list ap;
  va_start (ap, n);
  int s = 0;
  for (int i = 0; i < n; ++i)
    s += va_arg (ap, int);
  va_end (ap);
  return s;
}

struct D : B {
  int sum (int n, ...) override
  {
    va_list ap;
    va_start (ap, n);
    int s = 100;
    for (int i = 0; i < n; ++i)
      s += va_arg (ap, int);
    va_end (ap);
    return s;
  }
};

[[gnu::noinline]] int call (B& r) { return r.sum (3, 1, 2, 3); }

int main ()
{
  B b;
  D d;
  if (call (b) != 6 || call (d) != 106)
    __builtin_abort ();
}
