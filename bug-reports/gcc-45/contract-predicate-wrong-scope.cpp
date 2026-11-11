// A function-contract-specifier written in the classic spelling -- after the
// parameter list of the return type rather than after the declarator -- is
// attached to the right function, but its predicate is parsed while the
// return type's parameter scope is still open.  A name in the predicate
// therefore resolves to a parameter of the return type: a parameter of a
// different function type, which has no instance at the call.
//
// [basic.scope.param]/1.1 gives the function's own parameters a scope
// reaching the end of the init-declarator, and the contract is inside that
// init-declarator, so the predicate must see them.  The return type's
// parameter-declaration-clause is a separate parameter scope and does not
// reach here at all.
//
// This is not the drop in contract-silently-dropped.cpp.  The contract here
// is attached and evaluated -- it is the evaluation that is meaningless.
//
// g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe \
//     -o a.out contract-predicate-wrong-scope.cpp
// ./a.out
//
// Expected: violations from contradiction and control only.  Then "done".
// Actual:   which_type and which_size report as well, so all four do.
//
// No headers beyond <cstdio>, so this file is close to its own preprocessed
// source; same_v is hand-rolled to avoid pulling in <type_traits>.

#include <cstdio>

static long callee (long) { return 0; }

template <class T, class U> constexpr bool same_v = false;
template <class T> constexpr bool same_v<T, T> = true;

// In each declaration the function's own parameter is `int i`, and the
// return type `long (*) (long)` declares its own parameter, also spelled
// `i`, of type `long`.  Only one `i` is in scope for the predicate per
// [basic.scope.param]/1.1, and it is the int.

// 1.  Which declaration does the name resolve to?  The predicate asserts it
//     is this function's `i`, an int.  The assertion fails: decltype (i) is
//     long, so the name found the return type's parameter.

long (*which_type (int i)) (long i) pre (same_v<decltype (i), int>);

long (*which_type (int i)) (long) { (void) i; return callee; }

// 2.  The same answer by a second route, in case decltype is thought to be
//     doing something unusual here.

long (*which_size (int i)) (long i) pre (sizeof (i) == sizeof (int));

long (*which_size (int i)) (long) { (void) i; return callee; }

// 3.  Proof that the contract is evaluated rather than dropped: the
//     predicate is a contradiction, false for every possible value of `i`,
//     so it must report.  A dropped contract cannot.  This is what separates
//     this defect from the one in contract-silently-dropped.cpp.

long (*contradiction (int i)) (long i) pre (i > 0 && i <= 0);

long (*contradiction (int i)) (long) { (void) i; return callee; }

// 4.  Control: an ordinary function, whose contract is parsed in the right
//     scope.  Called with -1, so it must report.  If this one is silent the
//     measurement is broken, not the compiler.

int control (int i) pre (i > 0);

int control (int i) { return i; }

// A predicate that merely reads the misresolved parameter is deliberately
// not measured here.  `pre (i > 0)` in this position compiles to a read of a
// stack slot the function never writes -- at -O0 on x86_64 the real argument
// goes to one slot and the comparison reads another -- so it can report
// either way from one run to the next.  The three rows above are
// deterministic and establish the same thing.

int main ()
{
  which_type (1);
  which_size (1);
  contradiction (1);
  control (-1);
  std::puts ("done");
}
