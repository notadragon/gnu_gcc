// A lambda's postcondition result name lives in the postcondition's own
// scope.  It conflicts only with the lambda's parameters and init-captures
// ([basic.scope.contract]/2), not with the enclosing function's parameters,
// and it hides an enclosing local of the same name only inside the
// postcondition: after the lambda-expression, the name finds the local again.
// A result name "_" is name-independent and conflicts with nothing.
//
// Mirrors: clang/test/Contracts/result-name-lambda-enclosing-scope.cpp and
// result-name-underscore.cpp in the llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

// The enclosing function's parameter is not the lambda's.
int p (int x)
{
  auto l = [] (int y) post (x: x > 0) { return y; };
  return x + l (1);
}

// The enclosing local is found again after the lambda.
int q ()
{
  int r = 1;
  auto l = [] (int y) post (r: r > 0) { return y; };
  return r + l (1);
}

int q2 ()
{
  int r = 2;
  auto l = [] (int y) post (r: r > 0) { return y; };
  auto m = [&] { return r; };
  return m () + l (1);
}

// ... including an outer lambda's simple capture or init-capture named in
// the call's arguments.
int w = [] (int y) {
  int x = y;
  return [x] (int z) post (x: x > 0) { return z; } (x);
} (1);

int n = [x = 1] {
  return [] (int y) post (x: x > 0) { return y; } (x);
} ();

// "_" is name-independent.
int u (int _) post (_: true) { return 1; }
int s = [_ = 0] (int y) post (_: true) { return y; } (1);
int s2 = [] (int _) post (_: true) { return 1; } (0);

// A block-scope function declaration's result name is not the enclosing
// function's parameter either.
int b (int x)
{
  int g (int y) post (x: x > 0);
  return x;
}

// After the lambda the name finds the local, not a namespace-scope one.
constexpr int r = 100;
constexpr int q5 ()
{
  int r = 5;
  auto l = [] (int y) post (r: r > 0) { return y; };
  return r + l (1);
}
static_assert (q5 () == 6);

int main ()
{
  if (p (1) != 2 || q () != 2 || q2 () != 3 || w != 1 || n != 1
      || u (0) != 1 || s != 1 || s2 != 1 || b (1) != 1)
    __builtin_abort ();
}
