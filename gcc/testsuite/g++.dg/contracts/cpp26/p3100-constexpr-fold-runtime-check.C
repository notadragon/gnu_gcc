// P3100: a runtime-only check -- a guard the constant evaluator skips -- is
// kept when GCC would fold a constexpr call that is not manifestly
// constant-evaluated (GCC-797): the constructor call initializing an
// automatic variable, or a call in an initializer, is not replaced by its
// value.  Under observe, a member function called in a base mem-initializer
// (ub:class.base.init.mem.fun), a pure virtual function called directly in
// its class's constructor (ub:class.abstract.pure.virtual.direct) and an
// assignment from an overlapping object (ub:expr.assign.overlap) are each
// reported once, as are the forms whose calls cannot be folded.  Manifestly
// constant evaluation is unchanged: a constexpr or static variable is
// constant-initialized without a report, and the static_asserts hold.
// (Clang: CLANG-751.)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun:observe,ub:expr.assign.overlap:observe,ub:class.abstract.pure.virtual.direct:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-prune-output "called from constructor" }

#include <contracts>
#include <cstdio>
#include <cstring>
namespace cs = std::contracts;

static int reports;
struct PureVirtual {};
void handle_contract_violation (const cs::contract_violation &v)
{
  ++reports;
  // Observe aborts after the handler returns from a pure virtual call, which
  // has no valid continuation: leave by an exception instead.
  if (std::strstr (v.comment (), "pure virtual"))
    throw PureVirtual {};
}

struct Base { int v; constexpr Base (int y) : v (y) { } };

// Folded: f () is a constant, so is the whole constructor call.
struct Derived : Base {
  int j;
  constexpr int f () const { return 7; }
  constexpr Derived () : Base (f ()), j (5) { }
};

// Control: the constructor depends on its argument.
struct Derived3 : Base {
  int j;
  constexpr int f () const { return 7; }
  constexpr Derived3 (int x) : Base (f () + x), j (5) { }
};

struct A {
  int v;
  constexpr virtual int g () const = 0;
  constexpr A () : v (g ()) { }		// resolved statically to A::g
};
constexpr int A::g () const { return 1; }
struct B : A { constexpr int g () const override { return 2; } };

struct A3 {
  int v;
  constexpr virtual int g () const = 0;
  constexpr A3 (int x) : v (g () + x) { }
};
constexpr int A3::g () const { return 1; }
struct B3 : A3 {
  constexpr B3 (int x) : A3 (x) { }
  constexpr int g () const override { return 2; }
};

union U { int a; short b; };
constexpr int overlap () { U u; u.a = 1; u.b = u.a; return u.b; }
constexpr int overlap3 (int x) { U u; u.a = x; u.b = u.a; return u.b; }
struct O { int v; constexpr O () : v (overlap ()) { } };

static_assert (Derived ().v == 7);
static_assert (B ().v == 1);
static_assert (overlap () == 1);

#define COUNT(name, ...)					\
  do {								\
    int before = reports;					\
    try { __VA_ARGS__; } catch (PureVirtual &) { }		\
    std::printf (name " %d\n", reports - before);		\
  } while (0)

int
main (int argc, char **)
{
  COUNT ("base-init", Derived d; (void) d);
  COUNT ("base-init-control", Derived3 d3 (argc); (void) d3);
  COUNT ("pure-virtual", B b; (void) b);
  COUNT ("pure-virtual-control", B3 b3 (argc); (void) b3);
  COUNT ("overlap-call", int r = overlap (); (void) r);
  COUNT ("overlap-ctor", O o; (void) o);
  COUNT ("overlap-control", int r3 = overlap3 (argc); (void) r3);
  COUNT ("constexpr-var", constexpr Derived cd; constexpr O co; (void) cd;
	 (void) co);
  COUNT ("static-var", static Derived sd; static O so; (void) sd; (void) so);
  return 0;
}
// { dg-output "base-init 1(\n|\r\n|\r)" }
// { dg-output "base-init-control 1(\n|\r\n|\r)" }
// { dg-output "pure-virtual 1(\n|\r\n|\r)" }
// { dg-output "pure-virtual-control 1(\n|\r\n|\r)" }
// { dg-output "overlap-call 1(\n|\r\n|\r)" }
// { dg-output "overlap-ctor 1(\n|\r\n|\r)" }
// { dg-output "overlap-control 1(\n|\r\n|\r)" }
// { dg-output "constexpr-var 0(\n|\r\n|\r)" }
// { dg-output "static-var 0(\n|\r\n|\r)" }
