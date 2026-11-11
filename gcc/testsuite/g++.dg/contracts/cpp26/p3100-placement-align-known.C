// P3100: a placement new into a local array at an offset that misaligns the
// object (ub:basic.align.object.alignment) under "observe" (the .store check,
// made in the middle end, under its noexcept form): the pointer is checked
// against the alignment of the type created, not against the alignment the
// array is known to have at that offset -- an over-aligned empty class
// (.nostore) and each initialization that stores (.store, once each) are
// caught; an aligned offset is not.
// Mirror: llvm_llvm-project clang/test/Contracts/Runnable/p3100-placement-align-known.cpp.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:basic.align.object.alignment:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
#include <new>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation (const cs::contract_violation& v) {
  if (v.kind () == cs::assertion_kind::implicit)
    ++viols;
  std::printf ("VIOL comment=[%s]\n", v.comment ());
}
struct alignas (4) S { };
struct T { int a; };
int main () {
  alignas (8) char s[16];
  new (s + 4) S ();
  new (s + 4) int (5);
  std::printf ("viols=%d\n", viols);
  new (s + 1) S ();
  new (s + 1) int (5);
  new (s + 1) T {5};
  std::printf ("viols=%d\n", viols);
  return 0;
}
// { dg-output "viols=0(\n|\r\n|\r)" }
// { dg-output "VIOL comment=\\\[misaligned pointer passed to a non-allocating placement new\\\](\n|\r\n|\r)" }
// { dg-output "VIOL comment=\\\[misaligned pointer access\\\](\n|\r\n|\r)" }
// { dg-output "VIOL comment=\\\[misaligned pointer access\\\](\n|\r\n|\r)" }
// { dg-output "viols=3" }
