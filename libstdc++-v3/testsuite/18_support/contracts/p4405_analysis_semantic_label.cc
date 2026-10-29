// { dg-options "-fcontracts-p3850" }
// { dg-do compile { target c++26 } }

// D4405 ("P3400R5 Analysis Evaluation Configuration"): the
// analysis_semantic_label concept -- an assertion-control object whose
// analysis_semantic(), callable on a const object with no argument, is
// convertible to evaluation_semantic -- and a combined label, which models
// it when either constituent does and returns the stronger of their
// analysis semantics (ignore, then diagnose, then prove).  all() and
// operator~ hold every enumerator but unspecified.
//
// Mirror: libcxx/test/std/contracts/p4405_analysis_semantic_label.compile.pass.cpp
// in the llvm_llvm-project fork.

#include <contracts>

namespace L = std::contracts::labels;
using L::operator|;
using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template<evaluation_semantic S>
struct analyzed {
  using assertion_control_object = analyzed;
  constexpr evaluation_semantic analysis_semantic() const { return S; }
};

struct not_const {
  using assertion_control_object = not_const;
  constexpr evaluation_semantic analysis_semantic() { return prove; }
};

struct takes_argument {
  using assertion_control_object = takes_argument;
  constexpr evaluation_semantic
  analysis_semantic(evaluation_semantic s) const { return s; }
};

struct returns_int {
  using assertion_control_object = returns_int;
  constexpr int analysis_semantic() const { return 8; }
};

class hidden {
  constexpr evaluation_semantic analysis_semantic() const { return prove; }
public:
  using assertion_control_object = hidden;
};

struct no_aco {
  constexpr evaluation_semantic analysis_semantic() const { return prove; }
};

constexpr analyzed<ignore> a_ignore{};
constexpr analyzed<diagnose> a_diagnose{};
constexpr analyzed<prove> a_prove{};

static_assert(L::analysis_semantic_label<analyzed<diagnose>>);
static_assert(!L::analysis_semantic_label<not_const>);
static_assert(!L::analysis_semantic_label<takes_argument>);
static_assert(!L::analysis_semantic_label<returns_int>);
static_assert(!L::analysis_semantic_label<hidden>);
static_assert(!L::analysis_semantic_label<no_aco>);
static_assert(!L::analysis_semantic_label<L::empty_label_t>);
static_assert(!L::analysis_semantic_label<decltype(L::terminating)>);

// A combined label models the concept if either constituent does.
static_assert(L::analysis_semantic_label<decltype(a_diagnose
						   | L::terminating)>);
static_assert(L::analysis_semantic_label<decltype(L::terminating
						   | a_diagnose)>);
static_assert(!L::analysis_semantic_label<decltype(L::terminating
						    | L::symbolic)>);

// One constituent's value, and the stronger of two.
static_assert((a_diagnose | L::empty_label).analysis_semantic() == diagnose);
static_assert((L::empty_label | a_prove).analysis_semantic() == prove);
static_assert((a_ignore | a_diagnose).analysis_semantic() == diagnose);
static_assert((a_diagnose | a_ignore).analysis_semantic() == diagnose);
static_assert((a_prove | a_diagnose).analysis_semantic() == prove);
static_assert((a_ignore | a_prove).analysis_semantic() == prove);
static_assert((a_ignore | a_ignore).analysis_semantic() == ignore);
static_assert((a_diagnose | a_prove | a_ignore).analysis_semantic() == prove);

// all() and ~: every enumerator but unspecified.
static_assert(evaluation_semantic_set::all()
	      == evaluation_semantic_set{ignore, observe, enforce,
					 quick_enforce, assume,
					 noexcept_observe, noexcept_enforce,
					 prove, diagnose});
static_assert(!evaluation_semantic_set::all().contains(unspecified));
static_assert(~evaluation_semantic_set{} == evaluation_semantic_set::all());
