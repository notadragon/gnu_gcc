// In constant evaluation, a postcondition's result name for a class result
// constructed in place into a subobject designates that subobject.
//
// Mirror: clang/test/Contracts/constexpr-result-name-subobject.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

struct S { int v; };
constexpr S make (const int x) post (r : r.v == x) { return S{x}; }
struct Outer { int a; S s; };
constexpr Outer o{7, make (2)}; // OK
static_assert (o.s.v == 2);
constexpr S arr[2] = {make (3), make (4)}; // OK
static_assert (arr[1].v == 4);

constexpr S make7 (const int x) post (r : r.v == 7) { return S{x}; } // { dg-error "contract predicate is false in constant expression" }
constexpr Outer o2{7, make7 (5)};
