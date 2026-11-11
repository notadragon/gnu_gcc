// P3100: a misaligned placement new that stores nothing
// (ub:basic.align.object.alignment.nostore) under "observe": caught for a
// scalar and a trivial class left uninitialized, an over-aligned empty class
// whatever its initializer, and an array; not for an aligned pointer, a
// null one, a type of alignment 1, or an initialization that stores (the
// store is the .store leaf's).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.nostore:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
#include <new>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation (const cs::contract_violation& v) {
  if (v.kind () == cs::assertion_kind::implicit)
    ++viols;
}
struct S { int a, b; };
struct alignas (8) E { };
alignas (16) char buf[64];
void *volatile odd = buf + 1;
void *volatile even = buf + 8;
void *volatile nowhere = nullptr;
int main () {
  new (even) int;
  new (even) S;
  new (even) E;
  new (odd) char;
  new (nowhere) int;
  new (odd) int (5);
  new (odd) S ();
  std::printf ("viols=%d\n", viols);
  new (odd) int;
  new (odd) S;
  new (odd) E;
  new (odd) E ();
  new (odd) int[2];
  std::printf ("viols=%d\n", viols);
  return 0;
}
// { dg-output "viols=0(\n|\r\n|\r)viols=5" }
