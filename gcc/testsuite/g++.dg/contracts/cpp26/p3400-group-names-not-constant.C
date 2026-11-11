// P3400 [support.contract.control.ident] and notadragon_wg21 DECISIONS.md
// E189 (a): a label whose group_names is a range of names, but whose reading
// is not a constant expression, is an error, followed by the evaluator's
// account of why: silently giving it no groups would configure the
// assertion as unlabelled (here: enforced rather than ignored).  A
// group_names that is not such a range stays undiagnosed, as an absent
// one does: the label simply has no groups.  The operation limit is
// p3400-group-names-ops-limit.C.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce -fcontract-group-evaluation-semantic=safety:ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

constexpr const char *names[2] = { "a", "safety" };

// begin () has undefined behaviour: it reads past the end of names.
struct ub_range {
  constexpr const char *const *begin () const
  { return names + (names[2] != nullptr); }
  constexpr const char *const *end () const { return names + 2; }
};

struct ub_t {
  using assertion_control_object = ub_t;
  static constexpr ub_range group_names{};
};
constexpr ub_t ub{};

// A name that is not a constant: the array is not constexpr.
extern const char *const runtime_names[1];

struct runtime_range {
  constexpr const char *const *begin () const { return runtime_names; }
  constexpr const char *const *end () const { return runtime_names + 1; }
};

struct runtime_t {
  using assertion_control_object = runtime_t;
  static constexpr runtime_range group_names{};
};
constexpr runtime_t runtime_name{};

// Not a range of names: no groups, no diagnostic.
struct ints_t {
  using assertion_control_object = ints_t;
  static constexpr int group_names[2] = { 1, 2 };
};
constexpr ints_t ints{};

struct scalar_t {
  using assertion_control_object = scalar_t;
  static constexpr int group_names = 3;
};
constexpr scalar_t scalar{};

// A good one, for contrast.
struct good_t {
  using assertion_control_object = good_t;
  static constexpr const char *group_names[2] = { "a", "safety" };
};
constexpr good_t good{};

void f1 (int x) pre<ub> (x > 0) {}		// { dg-error "'group_names' facet of assertion-control object 'ub_t' must be usable in a constant expression" }
void f2 (int x) pre<runtime_name> (x > 0) {}	// { dg-error "'group_names' facet of assertion-control object 'runtime_t' must be usable in a constant expression" }
void f3 (int x) pre<ints> (x > 0) {}
void f4 (int x) pre<scalar> (x > 0) {}
void f5 (int x) pre<good> (x > 0) {}

// The evaluator's reasons.
// { dg-error "outside the bounds of array" "" { target *-*-* } 0 }
// { dg-error "'runtime_names' is not usable in a constant expression" "" { target *-*-* } 65 }
// { dg-message "'runtime_names' was not declared 'constexpr'" "" { target *-*-* } 31 }
