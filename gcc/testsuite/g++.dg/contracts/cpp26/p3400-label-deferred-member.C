// P3400: a label's assertion_control_object structural requirement must be
// checked for an in-class-defined member function's contract, not just for
// free functions.  Regression test: gating the check in grok_contract on the
// contract's condition not being a DEFERRED_PARSE node skips it entirely for
// an in-class-defined member function, which always has a deferred condition
// (only the predicate is deferred, never the label) -- silently, and even
// though the identical invalid label correctly errors on a
// free function.
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
