// P3098: Postcondition captures — redeclaration matching.
// Captures must match across declarations.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098" }

// Matching declarations — OK.
int f1(int i) post [i] (r: r >= i);
int f1(int i) post [i] (r: r >= i);

int f2(int i) post [old = i] (r: r >= old);
int f2(int i) post [old = i] (r: r >= old);

// Mismatched: one has captures, other does not.
// The predicate references different names so condition mismatch fires first.
int g1(int i) post [i] (r: r >= i);
int g1(int i) post (r: r >= 0); // { dg-error "mismatched contract condition" }

// Mismatched: different number of captures (same predicate text).
int g2(const int i, const int j) post [i] (r: r >= i);
int g2(const int i, const int j) post [i, j] (r: r >= i); // { dg-error "mismatched postcondition captures" }

// Mismatched: different initializer expression (predicate doesn't use capture).
int g3(int i) post [x = i] (r: r > 0);
int g3(int i) post [x = i + 1] (r: r > 0); // { dg-error "mismatched postcondition captures" }

// Mismatched: different capture names (condition mismatch fires because
// predicate uses different capture variable names).
int g4(int i) post [a = i] (r: r >= a);
int g4(int i) post [b = i] (r: r >= b); // { dg-error "mismatched contract condition" }
