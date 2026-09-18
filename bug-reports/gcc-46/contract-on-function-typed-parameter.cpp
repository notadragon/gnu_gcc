// GCC-46: a contract specifier written on a parameter of function type is
// accepted and silently dropped.
//
// A parameter of function type is adjusted to a pointer to function
// ([dcl.fct]/5), so the parameter declares no function and there is nothing
// for the contract to belong to -- no call through the pointer is ever
// checked against it.
//
// g++ -std=c++26 -fcontracts -fsyntax-only contract-on-function-typed-parameter.cpp
//
// Expected: rejected.  Actual: accepted with no diagnostic at all.

void takes_fn (int bar () pre (true));

void takes_fn_post (int bar () post (r : r > 0));

// With a parameter list of its own, so it is not specific to the
// no-parameter shape.
void takes_fn_args (int bar (int, int) pre (true));

// A definition rather than a declaration.
void defines_fn (int bar () pre (true)) { (void) bar; }

// In a member function, and in a template, so neither path is special.
struct S {
  void mem (int bar () pre (true));
};

template <typename T>
void tmpl (T bar () pre (true)) { (void) bar; }

template void tmpl<int> (int bar ());
