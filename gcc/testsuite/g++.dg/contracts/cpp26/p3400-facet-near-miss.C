// P3400: warn when a control object has a member that almost provides a facet.
//
// A facet is absent when its concept is not satisfied, and absence is silent
// by design -- p3400-facet-inaccessible.C pins that.  Silence is right for a
// type that never meant to provide the facet, but it is a poor answer for one
// that plainly did and got a detail wrong: the label compiles, the contract
// compiles, and the handler simply never runs.  The two ways to get it wrong
// that are worth a diagnostic are an inaccessible member and a non-const one.
//
// The warning must not fire on a member that merely shares a facet's name.
// D3400R5 is explicit that a label may carry private helpers named after a
// facet -- tag-dispatch overloads, say -- so the test for "meant to be a facet"
// is signature-based: relax exactly one thing (access, or constness) and ask
// whether that alone makes the call viable.  A helper with a different
// signature is viable under neither relaxation, so it stays silent.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstddef>

using std::contracts::contract_violation;
using std::contracts::evaluation_semantic;

// --------------------------------------------------------------------------
// Inaccessible members: the signature fits, only access stands in the way.
// --------------------------------------------------------------------------

struct priv_handler_t {
  using assertion_control_object = priv_handler_t;
private:
  void handle_contract_violation (const contract_violation&) const { }
};
constexpr priv_handler_t priv_handler{};

struct priv_query_t {
  using assertion_control_object = priv_query_t;
private:
  void* query (const void*, std::size_t) const { return nullptr; }
};
constexpr priv_query_t priv_query{};

struct priv_comment_t {
  using assertion_control_object = priv_comment_t;
private:
  constexpr const char* compute_comment (const char*) const { return "x"; }
};
constexpr priv_comment_t priv_comment{};

struct priv_message_t {
  using assertion_control_object = priv_message_t;
private:
  constexpr const char* compute_message (const char*) const { return "x"; }
};
constexpr priv_message_t priv_message{};

struct priv_semantic_t {
  using assertion_control_object = priv_semantic_t;
private:
  constexpr evaluation_semantic compute_semantic (evaluation_semantic s) const
  { return s; }
};
constexpr priv_semantic_t priv_semantic{};

void f1 (int x) pre<priv_handler>  (x > 0) { }  // { dg-warning "inaccessible" }
void f2 (int x) pre<priv_query>    (x > 0) { }  // { dg-warning "inaccessible" }
void f3 (int x) pre<priv_comment>  (x > 0) { }  // { dg-warning "inaccessible" }
void f4 (int x) pre<priv_message>  (x > 0) { }  // { dg-warning "inaccessible" }
void f5 (int x) pre<priv_semantic> (x > 0) { }  // { dg-warning "inaccessible" }

// --------------------------------------------------------------------------
// Non-const members: the facet is always invoked on a constexpr, therefore
// const, control object, so a non-const member can never be one.
// --------------------------------------------------------------------------

struct mut_handler_t {
  using assertion_control_object = mut_handler_t;
  void handle_contract_violation (const contract_violation&) { }
};
constexpr mut_handler_t mut_handler{};

struct mut_query_t {
  using assertion_control_object = mut_query_t;
  void* query (const void*, std::size_t) { return nullptr; }
};
constexpr mut_query_t mut_query{};

struct mut_comment_t {
  using assertion_control_object = mut_comment_t;
  constexpr const char* compute_comment (const char*) { return "x"; }
};
constexpr mut_comment_t mut_comment{};

struct mut_message_t {
  using assertion_control_object = mut_message_t;
  constexpr const char* compute_message (const char*) { return "x"; }
};
constexpr mut_message_t mut_message{};

struct mut_semantic_t {
  using assertion_control_object = mut_semantic_t;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic s)
  { return s; }
};
constexpr mut_semantic_t mut_semantic{};

void g1 (int x) pre<mut_handler>  (x > 0) { }  // { dg-warning "not 'const'" }
void g2 (int x) pre<mut_query>    (x > 0) { }  // { dg-warning "not 'const'" }
void g3 (int x) pre<mut_comment>  (x > 0) { }  // { dg-warning "not 'const'" }
void g4 (int x) pre<mut_message>  (x > 0) { }  // { dg-warning "not 'const'" }
void g5 (int x) pre<mut_semantic> (x > 0) { }  // { dg-warning "not 'const'" }

// --------------------------------------------------------------------------
// Not near misses.  Nothing below may warn.
// --------------------------------------------------------------------------

// A private helper that merely shares a facet's name.  Neither relaxation
// makes it viable, so the tag-dispatch idiom stays quiet.
struct dispatch_t {
  using assertion_control_object = dispatch_t;
private:
  void handle_contract_violation (int) const { }
  void* query (int) const { return nullptr; }
};
constexpr dispatch_t dispatch{};
void h1 (int x) pre<dispatch> (x > 0) { }

// A public member of the wrong shape, likewise.
struct wrong_shape_t {
  using assertion_control_object = wrong_shape_t;
  void handle_contract_violation () const { }
  constexpr int compute_comment (int) const { return 0; }
};
constexpr wrong_shape_t wrong_shape{};
void h2 (int x) pre<wrong_shape> (x > 0) { }

// Facets that are actually provided.
struct good_t {
  using assertion_control_object = good_t;
  void handle_contract_violation (const contract_violation&) const { }
  void* query (const void*, std::size_t) const { return nullptr; }
  constexpr const char* compute_comment (const char*) const { return "x"; }
  constexpr const char* compute_message (const char*) const { return "x"; }
  constexpr evaluation_semantic compute_semantic (evaluation_semantic s) const
  { return s; }
};
constexpr good_t good{};
void h3 (int x) pre<good> (x > 0) { }

// A static facet member is const-correct by construction.
struct static_t {
  using assertion_control_object = static_t;
  static void handle_contract_violation (const contract_violation&) { }
  static constexpr const char* compute_comment (const char*) { return "x"; }
};
constexpr static_t stat{};
void h4 (int x) pre<stat> (x > 0) { }

// A label with no facets at all has nothing to be near.
struct bare_t { using assertion_control_object = bare_t; };
constexpr bare_t bare{};
void h5 (int x) pre<bare> (x > 0) { }

// --------------------------------------------------------------------------
// Once per label type, not once per contract.
// --------------------------------------------------------------------------

void r1 (int x) pre<mut_handler> (x > 0) { }
void r2 (int x) pre<mut_handler> (x > 0) { }
void r3 (int x) post<mut_handler> (true) { (void) x; }
