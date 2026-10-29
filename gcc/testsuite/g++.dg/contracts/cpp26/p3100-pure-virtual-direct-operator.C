// P3100: a pure virtual operator of *this called in its class's constructor
// (ub:class.abstract.pure.virtual.direct) under a throwing "enforce": an
// operator() call, a subscript, a unary and a binary operator, a compound
// assignment and a conversion function, each resolved statically to the pure
// virtual, are caught, as a named member function call is; the qualified
// form and an operator that is not pure are not.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual.direct:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-prune-output "called from constructor" }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
struct Boom {};
static int viols, defs;
void handle_contract_violation (const cs::contract_violation& v) {
  if (v.kind () == cs::assertion_kind::implicit)
    ++viols;
  throw Boom {};
}

struct B {
  B (int k) {
    switch (k)
      {
      case 1: (*this) (); break;
      case 2: (void) (*this)[0]; break;
      case 3: (void) -*this; break;
      case 4: (void) (*this + 1); break;
      case 5: *this += 1; break;
      case 6: (void) static_cast<int> (*this); break;
      case 7: B::operator() (); break;	// qualified: not checked
      case 8: (void) !*this; break;		// not pure
      }
  }
  virtual void operator() () = 0;
  virtual int operator[] (int) = 0;
  virtual int operator- () = 0;
  virtual int operator+ (int) = 0;
  virtual B &operator+= (int) = 0;
  virtual operator int () = 0;
  virtual bool operator! () { return false; }
  virtual ~B () { }
};
void B::operator() () { ++defs; }
int B::operator[] (int) { return ++defs; }
int B::operator- () { return ++defs; }
int B::operator+ (int) { return ++defs; }
B &B::operator+= (int) { ++defs; return *this; }
B::operator int () { return ++defs; }

struct D : B {
  D (int k) : B (k) { }
  void operator() () override { }
  int operator[] (int) override { return 0; }
  int operator- () override { return 0; }
  int operator+ (int) override { return 0; }
  D &operator+= (int) override { return *this; }
  operator int () override { return 0; }
};

static void run (int k) { try { D d (k); } catch (Boom &) { } }

int main () {
  run (7);
  run (8);
  std::printf ("clean=%d defs=%d\n", viols, defs);
  for (int k = 1; k <= 6; ++k)
    run (k);
  std::printf ("caught=%d defs=%d\n", viols, defs);
  return 0;
}
// { dg-output "clean=0 defs=1(\n|\r\n|\r)caught=6 defs=1" }
