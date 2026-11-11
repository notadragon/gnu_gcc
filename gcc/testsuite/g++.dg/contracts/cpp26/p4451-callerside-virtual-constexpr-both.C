// P3097, DECISIONS.md E180 (5), in constant evaluation: the checks a
// virtual call makes of the contract assertions of the function it names
// never add ignore to their allowed sets, so with ignore configured
// everywhere -- also for caller-side checks (p4451-callerside-virtual.json)
// -- B::f's terminating precondition and postcondition still fail the
// evaluation of a call through a B reference to D::f, and only the
// ordinary precondition is skipped.  The constant-evaluation half of
// p4451-callerside-virtual.C.  Under -fcontracts-p3400 and
// -fcontracts-p4451 (the P3400 label); a copy of
// p4451-callerside-virtual-constexpr.C, since DejaGnu reads the expected
// diagnostics only from the file it runs.
//
// Mirror: clang/test/Contracts/p4451-callerside-virtual-constexpr.cpp in
// the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontracts-p4451 -fcontracts-p3097 -fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4451-callerside-virtual.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct B
{
  constexpr virtual int f (int x) const
    pre<terminating> (x > 0)		// { dg-error "contract predicate is false" }
    pre (x > 1)
    post<terminating> (r : r < 10)	// { dg-error "contract predicate is false" }
  { return x; }
  constexpr virtual ~B () = default;
};

struct D : B
{
  constexpr int f (int x) const override { return x; }
};

constexpr int
call (int x)
{
  D d;
  const B &b = d;
  return b.f (x);
}

constexpr int a = call (1);	// the ordinary precondition is skipped
constexpr int b = call (-1);
constexpr int c = call (20);
