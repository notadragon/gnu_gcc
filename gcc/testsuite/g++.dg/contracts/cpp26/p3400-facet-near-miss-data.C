// P3400: the near-miss facet warning for a member that is both inaccessible
// and non-const, and for the data-member facets allowed_semantics and
// group_names (notadragon_wg21 DECISIONS.md E61).  A data member is reported
// when it is inaccessible, not const, or both, and its type still has the
// facet's shape: convertible to evaluation_semantic_set for allowed_semantics,
// indexable to something convertible to const char * for group_names.  A
// same-named member of another shape, or a member function of that name,
// stays silent, as p3400-facet-near-miss.C requires for the function facets.
//
// Mirror: clang/test/Contracts/p3400-facet-near-miss-data.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

// A function facet that fails both ways at once.
struct both_comment_t {
  using assertion_control_object = both_comment_t;
private:
  constexpr const char* compute_comment (const char*) { return "x"; }  // { dg-message "declared here" }
};
constexpr both_comment_t both_comment{};
void f1 (int x) pre<both_comment> (x > 0) { }  // { dg-warning "does not provide the 'compute_comment' facet because that member is inaccessible and not 'const'" }

// allowed_semantics: non-const (a static member without constexpr or const),
// inaccessible, and both.
struct mut_allowed_t {
  using assertion_control_object = mut_allowed_t;
  static inline evaluation_semantic_set allowed_semantics  // { dg-message "declared here" }
    = {evaluation_semantic::ignore};
};
constexpr mut_allowed_t mut_allowed{};
void f2 (int x) pre<mut_allowed> (x > 0) { }  // { dg-warning "does not provide the 'allowed_semantics' facet because that member is not 'const'" }

struct priv_allowed_t {
  using assertion_control_object = priv_allowed_t;
private:
  static constexpr evaluation_semantic allowed_semantics  // { dg-message "declared here" }
    = evaluation_semantic::ignore;
};
constexpr priv_allowed_t priv_allowed{};
void f3 (int x) pre<priv_allowed> (x > 0) { }  // { dg-warning "does not provide the 'allowed_semantics' facet because that member is inaccessible" }

class both_allowed_t {
public:
  using assertion_control_object = both_allowed_t;
private:
  evaluation_semantic_set allowed_semantics = {};  // { dg-message "declared here" }
};
constexpr both_allowed_t both_allowed{};
void f4 (int x) pre<both_allowed> (x > 0) { }  // { dg-warning "does not provide the 'allowed_semantics' facet because that member is inaccessible and not 'const'" }

// group_names: non-const, inaccessible.
struct mut_group_t {
  using assertion_control_object = mut_group_t;
  static inline const char* group_names[] = {"g"};  // { dg-message "declared here" }
};
constexpr mut_group_t mut_group{};
void f5 (int x) pre<mut_group> (x > 0) { }  // { dg-warning "does not provide the 'group_names' facet because that member is not 'const'" }

struct priv_group_t {
  using assertion_control_object = priv_group_t;
private:
  static constexpr char group_names[][2] = {"g"};  // { dg-message "declared here" }
};
constexpr priv_group_t priv_group{};
void f6 (int x) pre<priv_group> (x > 0) { }  // { dg-warning "does not provide the 'group_names' facet because that member is inaccessible" }

// Same names, other shapes or kinds: silent.
struct other_shapes_t {
  using assertion_control_object = other_shapes_t;
  int allowed_semantics = 0;	// not convertible to evaluation_semantic_set
  int group_names = 0;		// not indexable
};
constexpr other_shapes_t other_shapes{};
void f7 (int x) pre<other_shapes> (x > 0) { }

struct other_kinds_t {
  using assertion_control_object = other_kinds_t;
private:
  int allowed_semantics () const { return 0; }
  struct group_names { };
};
constexpr other_kinds_t other_kinds{};
void f8 (int x) pre<other_kinds> (x > 0) { }
