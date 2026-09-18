// P3098: Postcondition captures — name lookup rules.
// Capture initializers see parameter scope (const-ified), not other captures.
// Predicate sees captures (shadowing parameters).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098" }

// Capture initializer sees the parameter i, not a hypothetical capture i.
int f1(int i)
  post [a = 3, b = i] (r: r > b || r > a); // b inits from param i

// Capture shadows parameter in predicate — no "must be const" error.
int f2(int i)
  post [i] (r: r >= i);  // predicate's i is the capture

// Uncaptured non-const param in predicate still errors.
int f3(int i, int j)
  post [i] (r: r >= j);  // { dg-error "must be const" }

// Init-capture initializer sees const-ified outer scope.
int mutable_global = 0;
void f4()
  post [x = (++mutable_global, 0)] (true); // { dg-error "increment of read-only" }

// Capture does not see other captures — `a` in b's init is the parameter.
int f5(int a)
  post [a, b = a] (r: r > b); // b = param a (not capture a)

// Captures are NOT const-ified (P3098 Section 4.4.1) — mutation is allowed.
int f6(int i)
  post [iter = i] (r: ++iter > 0); // OK: capture is mutable

// Capture of reference type is also not const-ified.
int f7(const int& x)
  post [v = x] (r: (v = 0, true)); // OK: capture v is mutable
