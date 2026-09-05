// GCC-27: two friend declarations of the same function whose contracts
// DISAGREE are accepted silently, where every other redeclaration path
// diagnoses the mismatch.
//
// Compile with: g++ -std=c++26 -fcontracts -fcontracts-p3850

struct C
{
  friend int f (int x) pre (x > 0);
  // The next line is accepted; it should draw "mismatched contract condition
  // in declaration", as the namespace-scope control below does.
  friend int f (int x) pre (x < 0);
};

int f (int x) { return x; }

// CONTROL: the identical mismatch outside a class IS diagnosed, which is what
// says the gap is specific to the deferred-parse path a friend declaration
// takes and not to contract matching in general.
//
//   int g (int x) pre (x > 0);
//   int g (int x) pre (x < 0);   // error: mismatched contract condition

int
main ()
{
  return f (1) == 1 ? 0 : 1;
}
