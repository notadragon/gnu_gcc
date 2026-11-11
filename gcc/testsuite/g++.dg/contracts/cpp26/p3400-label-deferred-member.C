// P3400: a label's assertion_control_object structural requirement must be
// checked for a member function's contract, not just for free functions.
// A function contract specifier's label is deferred with its predicate and
// checked where it is parsed -- for a member, when the class is complete
// (GCC-960) -- so a member's label is checked like a free function's,
// including one naming a member declared after it.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3400" }

struct not_a_label { int x; };
constexpr not_a_label bad_lbl{};

void freefn (int x) pre<bad_lbl> (x > 0) { } // { dg-error "does not satisfy" }

struct S
{
  void memfn (int x) pre<bad_lbl> (x > 0) { } // { dg-error "does not satisfy" }
  int post_fn (int x) post<bad_lbl> (r: r > 0) { return x; } // { dg-error "does not satisfy" }
};

// An in-class declaration defers its predicate the same way; the label is
// checked on it, and again on the out-of-line redeclaration (GCC-198).
struct T
{
  void memdecl (int x) pre<bad_lbl> (x > 0); // { dg-error "does not satisfy" }
  int post_decl (int x) post<bad_lbl> (r: r > 0); // { dg-error "does not satisfy" }
};

void T::memdecl (int x) pre<bad_lbl> (x > 0) { } // { dg-error "does not satisfy" }
int T::post_decl (int x) post<bad_lbl> (r: r > 0) { return x; } // { dg-error "does not satisfy" }

// A member of a class template: a non-dependent label is checked at
// definition time.
template<class U>
struct TT
{
  void m (U x) pre<bad_lbl> (x > 0); // { dg-error "does not satisfy" }
};

// A label naming a member declared later is checked at class completion.
struct L
{
  void m (int x) pre<later> (x > 0) { }	// { dg-error "does not satisfy" }
  static constexpr not_a_label later{};
};
