// P3100: with the direct pure-virtual call check configured
// (ub:class.abstract.pure.virtual.direct), constant evaluation is unchanged:
// a constexpr pure virtual with a definition, called directly in its class's
// constructor, still runs the definition in a constant expression.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual.direct:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-prune-output "called from constructor" }

struct B {
  int v = 0;
  constexpr B () { v = f (); }
  constexpr virtual int f () = 0;
  constexpr virtual ~B () { }
};
constexpr int B::f () { return 1; }
struct D : B { constexpr int f () override { return 2; } };
constexpr int g () { D d; return d.v * 10 + d.f (); }
static_assert (g () == 12);
