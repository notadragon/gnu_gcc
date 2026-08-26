// P3400: the rethrow shortcut must survive -g.
//
// Regression: under -g every statement list carries DEBUG_BEGIN_STMT
// frontier markers, and the walk treated an unrecognized statement as
// unanalysable -- so the optimization silently stopped firing in any debug
// build.  Nothing caught it because every other test here compiles without
// -g; it surfaced only when a Compiler Explorer session, which always passes
// -g, showed identical code with and without the shortcut.
//
// The shapes below put markers where they matter: before the sole statement
// of a handler body, between the statements of a multi-statement body, and
// inside a callee the walk follows.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -g -fdump-tree-original" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::detection_mode;

bool boom ();

// 1. Single-statement body.
struct plain_t {
  using assertion_control_object = plain_t;
  void handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      throw;
  }
};
constexpr plain_t plain{};
int f1 (int i) pre<plain> (boom ()) { return i; }

// 2. Several statements, so markers sit between them as well.
struct via_local_t {
  using assertion_control_object = via_local_t;
  void handle_contract_violation (const contract_violation& v) const {
    const auto dm = v.detection_mode ();
    if (dm != detection_mode::evaluation_exception)
      return;
    throw;
  }
};
constexpr via_local_t via_local{};
int f2 (int i) pre<via_local> (boom ()) { return i; }

// 3. Markers inside a followed callee, not just the handler itself.
inline void rethrow_if_exception (const contract_violation& v) {
  if (v.detection_mode () == detection_mode::evaluation_exception)
    throw;
}

struct delegates_t {
  using assertion_control_object = delegates_t;
  void handle_contract_violation (const contract_violation& v) const {
    rethrow_if_exception (v);
  }
};
constexpr delegates_t delegates{};
int f3 (int i) pre<delegates> (boom ()) { return i; }

// All three checks are emitted, and none of them catches the predicate's
// exception even though the markers are present.
// { dg-final { scan-tree-dump-times "__cxa_contract_violation_pre_enforce_pf" 3 "original" } }
// { dg-final { scan-tree-dump-not "__cxa_contract_violation_pre_enforce_ex" "original" } }
