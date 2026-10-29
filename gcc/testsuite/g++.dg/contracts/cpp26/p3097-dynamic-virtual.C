// P3097 x P3595: callee-side dynamic selection reaches both halves of a
// virtual call.  The statically chosen function's assertions (checked in the
// interface wrapper) and the final overrider's own are configured to
// different dynamic selectors, chosen by namespace, and each selector's
// result is read at run time, per call: switching either one changes which
// of the two preconditions is checked, without recompiling.  A violation
// reports the function whose assertion failed.
//
// Mirror: clang/test/Contracts/Runnable/p3097-dynamic-virtual.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3097-dynamic-virtual.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

using std::contracts::evaluation_semantic;

static evaluation_semantic iface_sem, impl_sem;
namespace sel {
  evaluation_semantic iface () { return iface_sem; }
  evaluation_semantic impl () { return impl_sem; }
}

static int violations;
static const char *where;
void handle_contract_violation (const std::contracts::contract_violation &v)
{
  ++violations;
  where = v.location ().function_name ();
}

namespace iface {
  struct B
  {
    virtual int f (int x) pre (x > 0) { return x; }
    virtual ~B () {}
  };
}
namespace impl {
  struct D : iface::B
  {
    int f (int x) override pre (x > 1) { return x; }
  };
}

static int
run (evaluation_semantic i, evaluation_semantic d, int x)
{
  iface_sem = i;
  impl_sem = d;
  violations = 0;
  where = "";
  impl::D obj;
  iface::B *p = &obj;
  p->f (x);
  return violations;
}

int
main ()
{
  const auto ign = evaluation_semantic::ignore;
  const auto obs = evaluation_semantic::observe;
  if (run (ign, ign, 0) != 0)
    __builtin_abort ();
  if (run (obs, ign, 0) != 1 || !std::strstr (where, "B::f"))
    __builtin_abort ();
  if (run (ign, obs, 0) != 1 || !std::strstr (where, "D::f"))
    __builtin_abort ();
  if (run (obs, obs, 0) != 2)
    __builtin_abort ();
  // x == 1 satisfies only the interface's precondition.
  if (run (obs, obs, 1) != 1 || !std::strstr (where, "D::f"))
    __builtin_abort ();
}
