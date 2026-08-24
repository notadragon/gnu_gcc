// P2900: within the predicate of a contract assertion, an id-expression
// naming a variable declared outside the predicate has const-qualified type.
// This must hold through a lambda that lexically appears in the predicate:
// a by-reference capture (implicit or explicit, at any nesting depth) of an
// outside-predicate entity is const, so it may not be mutated.  A by-value
// capture makes an in-predicate copy (mutable), which also "cuts the chain"
// for nested lambdas; an entity declared inside the predicate (a lambda-local)
// is not const.  The same rules apply in a templated function, where a
// dependent by-reference capture is const-qualified at instantiation.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

// A: implicit by-reference capture of an outside-predicate parameter is const;
// mutating it in the predicate is ill-formed.
void
a (int x)
{
  contract_assert ([&] { return ++x > 0; } ()); // { dg-error "increment of read-only reference" }
}

// E: explicit by-reference capture behaves the same as the implicit one.
void
e (int x)
{
  contract_assert ([&x] { return ++x > 0; } ()); // { dg-error "increment of read-only reference" }
}

// B: by-value capture makes an in-predicate mutable copy; not const.
void
b (int x)
{
  contract_assert ([=] () mutable { return ++x > 0; } ());
}

// C: the inner by-reference lambda captures the outer lambda's by-value
// copy, which was created inside the predicate; not const.
void
c (int x)
{
  contract_assert ([=] () mutable { return [&] { return ++x > 0; } (); } ());
}

// D: the inner by-reference lambda captures a predicate-local variable;
// not const.
void
d (int)
{
  contract_assert ([] { int x = 0; return [&] { return ++x > 0; } (); } ());
}

// G: a multi-level explicit by-reference chain reaching an outside-predicate
// parameter is const at every level.
void
g (int x)
{
  contract_assert ([&x] { return [&x] { return ++x > 0; } (); } ()); // { dg-error "increment of read-only reference" }
}

// H: same, with implicit by-reference captures.
void
h (int x)
{
  contract_assert ([&] { return [&] { return ++x > 0; } (); } ()); // { dg-error "increment of read-only reference" }
}

// FA: the const rule for an implicit by-reference capture of an
// outside-predicate parameter also holds in a templated function; the
// dependent capture is const-qualified when resolved at instantiation.
template<class T>
void
fa (T x)
{
  contract_assert ([&] { return ++x > 0; } ()); // { dg-error "increment of read-only reference" }
}
template void fa<int> (int);

// FB: by-value capture in a template; not const.
template<class T>
void
fb (T x)
{
  contract_assert ([=] () mutable { return ++x > 0; } ());
}
template void fb<int> (int);

// FC: nested value-then-reference in a template; not const.
template<class T>
void
fc (T x)
{
  contract_assert ([=] () mutable { return [&] { return ++x > 0; } (); } ());
}
template void fc<int> (int);

// FD: predicate-local in a template; not const.
template<class T>
void
fd (T)
{
  contract_assert ([] { int x = 0; return [&] { return ++x > 0; } (); } ());
}
template void fd<int> (int);
