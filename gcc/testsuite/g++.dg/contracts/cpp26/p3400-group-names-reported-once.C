// P3400 [support.contract.control.ident] and notadragon_wg21 DECISIONS.md
// E120, E189 (a), E190: a label whose group_names cannot be read -- a name
// that is not a constant, more than 4096 names -- is diagnosed once, at the
// label, also when the assertion is constant-evaluated, possibly more than
// once (GCC-981).  The names are read when the label is resolved, so the
// copy of the function's body that constant evaluation works on carries
// the result.  Each label is on a line of its own, and each message is
// matched with dg-regexp, which prunes one message only: a second report
// would be an excess error, on either line.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce -fcontract-group-evaluation-semantic=safety:ignore" }

extern const char *const rn[1];
struct rr {
  constexpr const char *const *begin () const { return rn; }
  constexpr const char *const *end () const { return rn + 1; }
};
struct lab {
  using assertion_control_object = lab;
  static constexpr rr group_names{};
};
constexpr lab l{};

// 4097 names: one more than a label may hold.
struct many_t {
  using assertion_control_object = many_t;
  static constexpr const char *group_names[4097] = { "a" };
};
constexpr many_t many{};

// An assertion statement, constant-evaluated twice.
constexpr int
f (int x)
{
  contract_assert<
    l
  > (x > 0);
  return 1;
}
static_assert (f (1));
static_assert (f (2));

// A precondition.
constexpr int
g (int x)
  pre<
    l
  > (x > 0)
{
  return 1;
}
static_assert (g (1));

// A prvalue label.
constexpr int
h (int x)
{
  contract_assert<
    lab{}
  > (x > 0);
  return 1;
}
static_assert (h (1));

// Too many names.
constexpr int
k (int x)
{
  contract_assert<
    many
  > (x > 0);
  return 1;
}
static_assert (k (1));

// Each label once, with the evaluator's account of why.
// { dg-regexp {[^\n]*:36:[0-9]+: error: the 'group_names' facet of assertion-control object 'lab' must be usable in a constant expression[^\n]*} }
// { dg-regexp {[^\n]*:36:[0-9]+: error: the value of 'rn' is not usable in a constant expression[^\n]*} }
// { dg-regexp {[^\n]*:13:[0-9]+: note: 'rn' was not declared 'constexpr'[^\n]*} }
// { dg-regexp {[^\n]*:47:[0-9]+: error: the 'group_names' facet of assertion-control object 'lab' must be usable in a constant expression[^\n]*} }
// { dg-regexp {[^\n]*:47:[0-9]+: error: the value of 'rn' is not usable in a constant expression[^\n]*} }
// { dg-regexp {[^\n]*:13:[0-9]+: note: 'rn' was not declared 'constexpr'[^\n]*} }
// { dg-regexp {[^\n]*:59:[0-9]+: error: the 'group_names' facet of assertion-control object 'lab' must be usable in a constant expression[^\n]*} }
// { dg-regexp {[^\n]*:59:[0-9]+: error: the value of 'rn' is not usable in a constant expression[^\n]*} }
// { dg-regexp {[^\n]*:13:[0-9]+: note: 'rn' was not declared 'constexpr'[^\n]*} }
// { dg-regexp {[^\n]*:70:[0-9]+: error: the 'group_names' facet of assertion-control object 'many_t' holds more than 4096 names[^\n]*} }
