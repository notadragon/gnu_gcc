// P3400: the rethrow shortcut is conservative.  None of these handlers is
// provably a bare rethrow of the in-flight exception, so every one of these
// checks keeps its try/catch and still calls the _ex entry point.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fdump-tree-original" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <exception>

using std::contracts::contract_violation;
using std::contracts::detection_mode;
using std::contracts::violation_handled;

bool boom ();
void log_it ();
extern int opaque;

// 1. Does something observable before rethrowing.
struct logs_first_t {
  using assertion_control_object = logs_first_t;
  void handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      {
	log_it ();
	throw;
      }
  }
};
constexpr logs_first_t logs_first{};
int f1 (int i) pre<logs_first> (boom ()) { return i; }

// 2. Returns instead of rethrowing: the exception would be swallowed, which
//    eliding the catch would not do.
struct swallows_t {
  using assertion_control_object = swallows_t;
  violation_handled
  handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      return violation_handled::handled;
    return violation_handled::not_handled;
  }
};
constexpr swallows_t swallows{};
int f2 (int i) pre<swallows> (boom ()) { return i; }

// 3. Raises a different exception -- not the same thing as never catching.
struct translates_t {
  using assertion_control_object = translates_t;
  void handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      throw 42;
  }
};
constexpr translates_t translates{};
int f3 (int i) pre<translates> (boom ()) { return i; }

// 4. The branch is not decidable at the point of the check.
struct runtime_choice_t {
  using assertion_control_object = runtime_choice_t;
  void handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception
	&& opaque)
      throw;
  }
};
constexpr runtime_choice_t runtime_choice{};
int f4 (int i) pre<runtime_choice> (boom ()) { return i; }

// 5. Handler declared here, defined after the guarded function: no body to
//    read when the check is emitted.
struct out_of_line_t {
  using assertion_control_object = out_of_line_t;
  void handle_contract_violation (const contract_violation& v) const;
};
constexpr out_of_line_t out_of_line{};
int f5 (int i) pre<out_of_line> (boom ()) { return i; }

void out_of_line_t::handle_contract_violation (const contract_violation& v) const
{
  if (v.detection_mode () == detection_mode::evaluation_exception)
    throw;
}

// 6. A noexcept handler cannot rethrow -- it would terminate.
struct noexcept_handler_t {
  using assertion_control_object = noexcept_handler_t;
  void handle_contract_violation (const contract_violation& v) const noexcept {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      log_it ();
  }
};
constexpr noexcept_handler_t noexcept_handler{};
int f6 (int i) pre<noexcept_handler> (boom ()) { return i; }

// 7. A contract with no label at all has no local handler to reason about.
int f7 (int i) pre (boom ()) { return i; }

// All seven keep both halves of the check.
// { dg-final { scan-tree-dump-times "__cxa_contract_violation_pre_enforce_pf" 7 "original" } }
// { dg-final { scan-tree-dump-times "__cxa_contract_violation_pre_enforce_ex" 7 "original" } }
