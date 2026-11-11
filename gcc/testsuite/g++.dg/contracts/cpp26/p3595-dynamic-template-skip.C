// P3595 dynamic selection: population must be SKIPPED in a dependent
// (uninstantiated) template body.  These templates are defined with
// dynamic-configured contracts but NEVER instantiated, so no dynamic descriptor
// or label transform table should be computed against the dependent types -- if
// it were, resolving the config / constant-evaluating the label facet on a
// dependent type would crash or emit spurious diagnostics.  A clean compile
// (no diagnostics) locks in that the dependent body is safely skipped.  The
// instantiation-side behaviour is covered by p3595-dynamic-template.C.
// (Clang mirror: clang/test/Contracts/p3595-dynamic-template-skip.cpp)
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-template-skip.json" }

namespace std::contracts {
enum class evaluation_semantic : unsigned char {
  ignore = 1, observe, enforce, quick_enforce
};
} // namespace std::contracts

// A label carrying a compute_semantic facet, whose evaluation on a dependent
// type would misbehave if population were not skipped.
struct to_observe_t {
  using assertion_control_object = to_observe_t;
  constexpr std::contracts::evaluation_semantic
  compute_semantic(std::contracts::evaluation_semantic __s) const {
    using enum std::contracts::evaluation_semantic;
    if (__s == enforce || __s == quick_enforce)
      return observe;
    return __s;
  }
};
constexpr to_observe_t to_observe{};

// Unlabeled dynamic contract in a dependent body.
template <class T>
void f(const T x) pre(x > 0) { }

// Labeled dynamic contract in a dependent body.
template <class T>
void g(const T x) pre<to_observe>(x > 0) { }

// contract_assert inside a dependent body, unlabeled and labeled.
template <class T>
void h(const T x) {
  contract_assert(x > 0);
  contract_assert<to_observe>(x > 0);
}

// Nothing is instantiated: no f<...>, g<...>, or h<...> is ever formed.
