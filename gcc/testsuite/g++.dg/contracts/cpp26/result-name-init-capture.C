// A lambda's postcondition result name cannot have the name of one of the
// lambda's init-captures ([basic.scope.contract]/2: a declaration whose target
// scope is the nearest enclosing lambda scope), beyond the plain case in
// result-name-shadows-parameter.C: an init-capture by reference, a pack
// init-capture, a generic lambda, a lambda in a function template.  A simple
// capture declares nothing; "_" is name-independent; an outer lambda's
// init-capture, and a function declared in a lambda's body, are not in the
// nearest enclosing lambda scope of the postcondition.  (EDG M16b shapes.)
//
// Mirror: clang/test/Contracts/result-name-init-capture.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=ignore" }

int v = 0;
auto b = [&r = v] (int y) post (r: r > 0) { return y; };  // { dg-error "result name shadows a lambda capture" }
auto c = [x = 1] (auto y) post (x: x > 0) { return y; };  // { dg-error "result name shadows a lambda capture" }

template <class... T> int pack (T... t)
{
  return [...xs = t] (int y) post (xs: xs > 0) { return y; } (1);  // { dg-error "result name shadows a lambda capture" }
}

template <class T> T in_template (T t)
{
  return [x = t] (T y) post (x: x > 0) { return y; } (t);  // { dg-error "result name shadows a lambda capture" }
}

// OK: "_" is name-independent.
int s = [_ = 0] (int y) post (_: true) { return y; } (1);

// OK: a simple capture declares nothing.
int w = [] (int y) {
  int x = y;
  return [x] (int z) post (x: x > 0) { return z; } (x);
} (1);

// OK: the outer lambda's init-capture is not in the inner lambda's scope.
int n = [x = 1] {
  return [] (int y) post (x: x > 0) { return y; } (x);
} ();

// OK: a function declared in the body has its own parameter scope.
int l = [x = 1] {
  int f (int) post (x: x > 0);
  return f (x);
} ();
