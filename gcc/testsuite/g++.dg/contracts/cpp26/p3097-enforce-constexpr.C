// P3097 x constant evaluation: under enforce, a violated assertion of a
// virtual call made during constant evaluation makes the program
// ill-formed -- the interface's precondition, checked by the wrapper, and
// the final overrider's own alike -- while the same calls outside constant
// evaluation are well-formed (they are checked, and terminate, at run time).
//
// Mirror: clang/test/Contracts/p3097-enforce-constexpr.cpp in the
// llvm_llvm-project fork; owed to EDG, recorded in its open-issues/README.md.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontract-evaluation-semantic=enforce" }

struct B
{
  constexpr virtual int f (int x) const pre (x > 0) { return x; } // { dg-error "contract predicate is false in constant expression" }
  constexpr virtual ~B () = default;
};

struct D : B
{
  constexpr int f (int x) const override pre (x > 1) { return x; } // { dg-error "contract predicate is false in constant expression" }
};

constexpr int
call (int x)
{
  D d;
  const B &b = d;
  return b.f (x);
}

constexpr int ok = call (2);
constexpr int bad_iface = call (0);	// interface precondition
constexpr int bad_own = call (1);	// D's own precondition

int
runtime (int x)
{
  return call (x);	// not constant evaluated: fine
}
