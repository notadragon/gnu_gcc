// P3100: a null pointer passed to a non-allocating placement new
// (ub:expr.new.non.allocating.null) under "observe": a null pointer is
// caught for a trivial type, an empty class, an array and `::new'; a
// non-null pointer, a class's own placement operator new and the nothrow
// form are not checked, nor is an unevaluated operand.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.new.non.allocating.null:observe" }
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
struct T { int x; };
struct E { };
struct U {
  int x;
  static void *operator new (std::size_t, void *p) noexcept { return p; }
};
void *volatile nowhere = nullptr;
static_assert (noexcept (new (nowhere) T));
int main () {
  alignas (int) char buf[4 * sizeof (int)];
  int r = 0;
  r += new (buf) int (5) == (int *) buf;
  r += new (buf) T == (T *) buf;
  r += new (nowhere) U == nullptr;		// class-specific: not checked
  T *n = new (std::nothrow) T;
  r += n != nullptr;
  delete n;
  std::printf ("OK=%d viols=%d\n", r, viols);
  new (nowhere) T;
  new (nowhere) E;
  new (nowhere) int[2];
  ::new (nowhere) T;
  std::printf ("viols=%d\n", viols);
  return 0;
}
// { dg-output "OK=4 viols=0(\n|\r\n|\r)viols=4" }
