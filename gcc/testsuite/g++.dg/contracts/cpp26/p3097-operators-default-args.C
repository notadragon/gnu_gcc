// P3097: a virtual call through operator syntax goes through the interface
// wrapper like any other virtual call, so the statically chosen operator's
// assertions are checked around the final overrider's own: call, subscript,
// comparison, compound assignment, unary minus and conversion operators,
// each called on a base reference, each with an interface precondition (or,
// for the conversion, postcondition) that fails and an override with none.
//
// Default arguments are taken from the statically chosen function
// ([dcl.fct.default]), and the wrapper must not change that: through a B
// the argument is B's default, which satisfies B's precondition and violates
// D's own; through a D it is D's default, which satisfies both.
//
// Mirror: clang/test/Contracts/Runnable/p3097-operators-default-args.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

static int violations;
static const char *where;
void handle_contract_violation (const std::contracts::contract_violation &v)
{
  ++violations;
  where = v.location ().function_name ();
}

static bool
reported (int n, const char *fn)
{
  bool ok = violations == n && std::strstr (where, fn);
  violations = 0;
  where = "";
  return ok;
}

struct B
{
  virtual int operator() (int x) pre (x > 0) { return x; }
  virtual int operator[] (int x) pre (x > 0) { return x; }
  virtual bool operator== (int x) const pre (x > 0) { return x == 1; }
  virtual B &operator+= (int x) pre (x > 0) { return *this; }
  virtual int operator- () const pre (neg_ok) { return -1; }
  virtual explicit operator int () const post (r : r > 0) { return 1; }
  virtual int def (int x = 1) pre (x == 1) { return x; }
  bool neg_ok = true;
  virtual ~B () {}
};

struct D : B
{
  int operator() (int x) override { return x; }
  int operator[] (int x) override { return x; }
  bool operator== (int x) const override { return x == 1; }
  D &operator+= (int) override { return *this; }
  int operator- () const override { return -2; }
  explicit operator int () const override { return 0; }
  int def (int x = 2) override pre (x == 2) { return x; }
};

int
main ()
{
  D d;
  B &b = d;
  where = "";

  if (b (0) != 0 || !reported (1, "B::operator()"))
    __builtin_abort ();
  if (b[0] != 0 || !reported (1, "B::operator[]"))
    __builtin_abort ();
  if ((b == 0) || !reported (1, "B::operator=="))
    __builtin_abort ();
  b += 0;
  if (!reported (1, "B::operator+="))
    __builtin_abort ();
  b.neg_ok = false;
  if (-b != -2 || !reported (1, "B::operator-"))
    __builtin_abort ();
  if (static_cast<int> (b) != 0 || !reported (1, "B::operator int"))
    __builtin_abort ();
  // Holding arguments: nothing reported.
  b (1);
  (void) b[1];
  b += 1;
  if (violations != 0)
    __builtin_abort ();

  // Through a B: B's default 1, which D's own precondition rejects.
  if (b.def () != 1 || !reported (1, "D::def"))
    __builtin_abort ();
  // Through a D: D's default 2; D::def is both the statically chosen
  // function and the final overrider, and its precondition holds.
  if (d.def () != 2 || violations != 0)
    __builtin_abort ();
}
