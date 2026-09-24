// A contract predicate that modifies an object of the enclosing constant
// evaluation is rejected as "contract condition is not constant".
//
// The predicate is a core constant expression, so the program is well
// formed; [basic.contract.eval] contains this exact construct as a worked
// example.  g++ -std=c++26 -fcontracts rejects all three cases below.
//
// No headers, so this file is also its own preprocessed source.

// ---------------------------------------------------------------- case 1
// The example from [basic.contract.eval], verbatim.  The standard's own
// comment says the array's size depends on the evaluation semantic, which
// presupposes the modification happens and is visible.
constexpr int f (int i)
{
  contract_assert ((++const_cast<int &> (i), true));
  return i;
}
inline void g ()
{
  int a[f (1)];   // { expected: size 2 under a checking semantic }
  (void) a;
}

// ---------------------------------------------------------------- case 2
// The same defect reached through a member function rather than a cast on a
// parameter.  const_cast is needed only to undo the const-ification that
// [basic.contract.general] applies to the id-expression; `s` is not a const
// object, so modifying it is well defined.
struct string {
  const char *p; int i;
  constexpr string (const char *p): p (p), i (0) { }
  constexpr int length () { ++i; return __builtin_strlen (p); }
};

constexpr int h ()
{
  string s ("foobar");
  contract_assert (const_cast<string &> (s).length () > 0);
  return s.i;     // { expected: 1 -- the predicate was evaluated }
}

static_assert (h () == 1);

// ---------------------------------------------------------------- case 3
// Runtime and constant evaluation disagree about the same function: built
// and run, h() returns 1; constant-evaluated, it is rejected outright.
int main () { return h (); }    // { expected: exit status 1 }
