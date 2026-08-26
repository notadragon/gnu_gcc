// P3400: codegen for the rethrow shortcut.  Each of these handlers provably
// rethrows an evaluation_exception, so no check wraps its predicate in a
// try/catch -- the _ex entry point, which is only ever called from that catch,
// is absent.  The predicate-false path is untouched.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fdump-tree-original" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <exception>

using std::contracts::contract_violation;
using std::contracts::detection_mode;
using std::contracts::violation_handled;

bool boom ();			// Not noexcept: the predicate might throw.

// 1. The canonical shape.
struct plain_t {
  using assertion_control_object = plain_t;
  void handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      throw;
  }
};
constexpr plain_t plain{};
int f1 (int i) pre<plain> (boom ()) { return i; }

// 2. Through a local variable, with the test inverted and an early return.
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

// 3. rethrow_exception (current_exception ()) reaches the same place.
struct via_ptr_t {
  using assertion_control_object = via_ptr_t;
  void handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      std::rethrow_exception (std::current_exception ());
  }
};
constexpr via_ptr_t via_ptr{};
int f3 (int i) pre<via_ptr> (boom ()) { return i; }

// 4. The semantic is known where the check is emitted too, so a handler that
//    also tests is_terminating () still resolves (enforce is terminating).
struct checks_terminating_t {
  using assertion_control_object = checks_terminating_t;
  void handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception
	&& v.is_terminating ())
      throw;
  }
};
constexpr checks_terminating_t checks_terminating{};
int f4 (int i) pre<checks_terminating> (boom ()) { return i; }

// 5. A handler returning violation_handled: the rethrow comes first, so the
//    return is unreachable on the exception path.
struct returns_handled_t {
  using assertion_control_object = returns_handled_t;
  violation_handled
  handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      throw;
    return violation_handled::not_handled;
  }
};
constexpr returns_handled_t returns_handled{};
int f5 (int i) pre<returns_handled> (boom ()) { return i; }

// Every check is still emitted ...
// { dg-final { scan-tree-dump-times "__cxa_contract_violation_pre_enforce_pf" 5 "original" } }
// ... and not one of them catches the predicate's exception.
// { dg-final { scan-tree-dump-not "__cxa_contract_violation_pre_enforce_ex" "original" } }
