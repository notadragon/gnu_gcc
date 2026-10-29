// The observe half of constexpr-dtor-contracts-constant-destruction.C: a
// constexpr variable's constant destruction, and a class-type template
// argument's, warn for a violated destructor precondition, or one that is
// not a core constant expression, and stay constant.  The destruction of
// any other variable whose destructor's precondition fails is not
// constant: its destructor is registered with __cxa_atexit, so the
// precondition is evaluated at run time.  GCC evaluates constant
// destruction only for a constexpr variable and registers the destructor
// of every other object that has a non-trivial one, so the non-constexpr
// `DT t1;', whose destruction would be constant, is registered too, where
// Clang destroys nothing (DECISIONS.md E149 (a), the Divergences row
// GCC-934); the constexpr `t2' is not.
//
// Mirror: clang/test/Contracts/constexpr-dtor-contracts-constant-destruction.cpp
// in the llvm_llvm-project fork (its observe and code-generation runs).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe -fuse-cxa-atexit -fdump-tree-gimple" }

struct DF {
  constexpr DF () {}
  constexpr ~DF () pre (false) {}	// { dg-warning "contract predicate is false in constant expression" }
};

int g;
struct DE {
  constexpr ~DE () pre (g > 0) {}	// { dg-warning "contract condition is not constant" }
};

struct DT {
  constexpr ~DT () pre (true) {}
};

constexpr DF f2;

template <DF x> struct X {};
X<DF{}> xv;

constexpr DE e2;

// Not constant destruction: destroyed at exit, through the destructor.
DE e1;
void f () { static DE el; }
DF ff;

DT t1;
constexpr DT t2;

// { dg-final { scan-tree-dump "__cxa_atexit \\(__dt_comp , &e1," "gimple" } }
// { dg-final { scan-tree-dump "__cxa_atexit \\(__dt_comp , &el," "gimple" } }
// { dg-final { scan-tree-dump "__cxa_atexit \\(__dt_comp , &ff," "gimple" } }
// { dg-final { scan-tree-dump "__cxa_atexit \\(__dt_comp , &t1," "gimple" } }
// { dg-final { scan-tree-dump-not "&f2," "gimple" } }
// { dg-final { scan-tree-dump-not "&e2," "gimple" } }
// { dg-final { scan-tree-dump-not "&t2," "gimple" } }
