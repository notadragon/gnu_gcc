// GCC-910 / PR127911: a VLA captured by reference in a lambda in a function
// template ICEs at instantiation in a C++26 dialect (contracts on by
// default), although no contract appears.
// g++ -std=c++26 -fsyntax-only gcc-910-vla-capture-template-ice.cpp
// Expected: clean.  Actual (stock trunk): internal compiler error: in
// set_contract_capture_flag, at cp/semantics.cc
typedef __INTPTR_TYPE__ intptr_t;

template <typename T> void foo (T n)
{
  intptr_t buf[n];
  [&buf] () { __typeof (buf) baz; } ();
}

void bar () { foo (42); }
