// A function-contract-specifier-seq follows the complete DECLARATOR, so it
// belongs to the function that declarator declares -- however complicated the
// declarator is, and whether or not the function's return type happens to
// contain a parameter list of its own.
//
// Every shape below is well-formed and its contract must be checked.  This
// test RUNS, and that is the point of it: a compile-only test cannot tell an
// attached contract from a silently discarded one, and a discarded contract
// is precisely the failure this file exists to catch.  The trailing-return
// shapes were accepted with no diagnostic and no check for the entire life of
// this branch, and a `dg-do compile` test of them would have passed
// throughout.
//
// Each call below violates exactly one contract, and expect_one names the
// shape, so a single dropped or misattached specifier is reported on its own
// rather than as a wrong total.
//
// Tracked as GCC-45 in this repository's bug-reports/, filed upstream as
// PR127572; the reproducers there are contract-silently-dropped.cpp and
// contract-predicate-wrong-scope.cpp.  Stock g++ 16.1.0, 16.2.0 and trunk
// check one of these shapes, drop two, and resolve the predicate of another
// against the return type's parameter.
//
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;
static int expected = 0;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

// Call this straight after a call that owes exactly one violation.  The
// semantic is `observe`, so the call returns and execution continues.
static void
expect_one (const char *what)
{
  ++expected;
  if (violations != expected)
    {
      __builtin_printf ("contract not checked: %s\n", what);
      __builtin_abort ();
    }
}

// Call this after a call whose contract must hold, so no violation is owed.
// A drop is indistinguishable from a satisfied predicate here, which is why
// every expect_none row below is paired with an expect_one row that inverts
// the same predicate.
static void
expect_none (const char *what)
{
  if (violations != expected)
    {
      __builtin_printf ("contract wrongly violated: %s\n", what);
      __builtin_abort ();
    }
}

static int callee (int) { return 0; }
static long callee_long (long) { return 0; }
static char buf[3];
static bool flag = false;

template <class T, class U> constexpr bool same_v = false;
template <class T> constexpr bool same_v<T, T> = true;

struct S { int m (int) { return 0; } };

// ---------------------------------------------------------------------------
// The declarator's outermost part is a parameter list belonging to the RETURN
// TYPE.  The contract follows the complete declarator, so it is the function's.
// ---------------------------------------------------------------------------

// Classic spelling, predicate naming the function's own parameter.  This is
// the row that shows the predicate must be parsed in the scope of the
// function's parameters and not after that scope has been left.
int (*b1 (int i)) (int) pre (i > 0);
int (*b1 (int i)) (int) { (void) i; return callee; }

// Classic spelling with a parameter-free predicate.  Separated from b1
// deliberately: this one is checked correctly by stock g++, so it is the
// control proving every other row here should be, and it is the row a
// too-broad "cannot appear on a return type" rule takes with it.
int (*b2 (int)) (int) pre (flag);
int (*b2 (int)) (int) { return callee; }

// A pointer to member function as the return type.
int (S::*b3 (int i)) (int) pre (i > 0);
int (S::*b3 (int i)) (int) { (void) i; return &S::m; }

// ---------------------------------------------------------------------------
// The parameter list sits inside a trailing-return type-id, so it belongs to
// an abstract declarator that never becomes a function.
// ---------------------------------------------------------------------------

auto b4 (int i) -> int (*) (int) pre (i > 0);
auto b4 (int i) -> int (*) (int) { (void) i; return callee; }

// A postcondition in the same position, so a fix that only handles `pre` is
// still caught.  The predicate reads a global rather than the parameter
// because a by-value parameter named in a postcondition must be const
// ([dcl.contract.func]), which is a separate rule and not what this row is
// for.
auto b5 (int) -> int (&) (int) post (r : flag);
auto b5 (int) -> int (&) (int) { return callee; }

// ---------------------------------------------------------------------------
// The declarator's outermost part is not a parameter list at all, so the
// parser has never looked for a contract here.
// ---------------------------------------------------------------------------

char (*b6 (int i)) [3] pre (i > 0);
char (*b6 (int i)) [3] { (void) i; return &buf; }

char (&b7 (int i)) [3] pre (i > 0);
char (&b7 (int i)) [3] { (void) i; return buf; }

// ---------------------------------------------------------------------------
// Controls.  All of these are checked today and must stay that way.
// ---------------------------------------------------------------------------

int c1 (int i) pre (i > 0);
int c1 (int i) { return i; }

auto c2 (int i) -> int pre (i > 0);
auto c2 (int i) -> int { return i; }

// c3 is b4 with the parameter list taken out of the type-id, and nothing
// else.  It works today, and that is what isolates the rule: the difference
// between a contract that is honoured and one that is silently dropped is
// whether the return type happens to contain a parameter list.
auto c3 (int i) -> char (*) [3] pre (i > 0);
auto c3 (int i) -> char (*) [3] { (void) i; return &buf; }

// The contract is written on a function-DEFINITION rather than on a prior
// declaration.  That is a separate grammar position from init-declarator and
// has to carry the seq too.
int (*defn_only (int i)) (int) pre (i > 0) { (void) i; return callee; }

// member-declarator, the third position, in both its forms: declared in the
// class and defined out of line, and defined in the class outright.  No
// virtual member here -- a contract on one is gated on P3097, which is a
// different rule, and virtual dispatch is covered elsewhere in this
// directory.
struct M {
  int (*mem (int i)) (int) pre (i > 0);
  int (*mem_inline (int i)) (int) pre (i > 0) { (void) i; return callee; }
};

int (*M::mem (int i)) (int) { (void) i; return callee; }

// lambda-declarator, the fourth position.
// ---------------------------------------------------------------------------
// WHICH parameter the predicate's name resolves to, when both the function
// and its return type declare one with the same spelling.
//
// In d1 and d2 the function's own parameter is `int i`, and the return type
// `long (*) (long)` declares its own `long i`.  [basic.scope.param]/1.1
// extends the function parameter scope to the end of the init-declarator, and
// the contract is inside it, so `i` is the function's `int` -- the return
// type's parameter-declaration-clause is a separate scope that does not reach
// here.  Stock g++ resolves to the `long`, compiles clean, and checks an
// object that was never created.
//
// The pair is complementary on purpose: d1 must be silent and d2 must report.
// A compiler that binds to the wrong parameter inverts both, and a compiler
// that drops the contract entirely fails d2 alone -- so neither row can pass
// for the wrong reason.
// ---------------------------------------------------------------------------

long (*d1 (int i)) (long i) pre (same_v<decltype (i), int>);
long (*d1 (int i)) (long) { (void) i; return callee_long; }

long (*d2 (int i)) (long i) pre (!same_v<decltype (i), int>);
long (*d2 (int i)) (long) { (void) i; return callee_long; }

static auto lam = [] (int i) -> int (*) (int) pre (i > 0)
                  { (void) i; return callee; };

template <class T>
auto tmpl (T i) -> int (*) (int) pre (i > T ()) { (void) i; return callee; }

template auto tmpl<int> (int) -> int (*) (int);

int
main ()
{
  b1 (-1); expect_one ("b1  classic, predicate names a parameter");
  b2 (0);  expect_one ("b2  classic, parameter-free predicate");
  b3 (-1); expect_one ("b3  classic, pointer-to-member return");
  b4 (-1); expect_one ("b4  trailing return, pointer to function");
  b5 (-1); expect_one ("b5  trailing return, reference to function, post");
  b6 (-1); expect_one ("b6  array declarator");
  b7 (-1); expect_one ("b7  reference-to-array declarator");

  c1 (-1); expect_one ("c1  plain function");
  c2 (-1); expect_one ("c2  trailing return, no parameter list in type-id");
  c3 (-1); expect_one ("c3  trailing return, array -- b4 minus the params");

  d1 (1);  expect_none ("d1  predicate names the function's parameter, not "
			"the return type's");
  d2 (1);  expect_one ("d2  the same, inverted");

  defn_only (-1);     expect_one ("function-definition position");

  M m;
  m.mem (-1);         expect_one ("member-declarator, defined out of line");
  m.mem_inline (-1);  expect_one ("member-declarator, defined in class");

  lam (-1);           expect_one ("lambda-declarator position");

  tmpl<int> (-1);     expect_one ("template");

  return 0;
}
