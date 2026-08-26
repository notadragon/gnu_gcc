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

// 8. Delegation does not launder a side effect: the helper is followed, and
//    fails inside the nested walk exactly as it would at the top level.
inline void log_then_rethrow () { log_it (); throw; }

struct delegates_to_logger_t {
  using assertion_control_object = delegates_to_logger_t;
  void handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      log_then_rethrow ();
  }
};
constexpr delegates_to_logger_t delegates_to_logger{};
int f8 (int i) pre<delegates_to_logger> (boom ()) { return i; }

// 9. A helper with no body available here cannot be followed.
void opaque_rethrow ();

struct delegates_out_of_line_t {
  using assertion_control_object = delegates_out_of_line_t;
  void handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      opaque_rethrow ();
  }
};
constexpr delegates_out_of_line_t delegates_out_of_line{};
int f9 (int i) pre<delegates_out_of_line> (boom ()) { return i; }

// 10. Mutual recursion terminates on the depth limit rather than hanging,
//     and yields no proof.
struct recursive_t {
  using assertion_control_object = recursive_t;
  void handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      handle_contract_violation (v);
  }
};
constexpr recursive_t recursive{};
int f10 (int i) pre<recursive> (boom ()) { return i; }

// 11. A combined label whose earlier component does something observable:
//     the rethrowing component is real, but it is not reached for free.
struct logs_and_declines_t {
  using assertion_control_object = logs_and_declines_t;
  violation_handled
  handle_contract_violation (const contract_violation&) const {
    log_it ();
    return violation_handled::not_handled;
  }
};
struct rethrows_t {
  using assertion_control_object = rethrows_t;
  violation_handled
  handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      throw;
    return violation_handled::not_handled;
  }
};
constexpr logs_and_declines_t logs_and_declines{};
constexpr rethrows_t rethrows{};
int f11 (int i) pre<logs_and_declines | rethrows> (boom ()) { return i; }

// All eleven keep both halves of the check.
// { dg-final { scan-tree-dump-times "__cxa_contract_violation_pre_enforce_pf" 11 "original" } }
// { dg-final { scan-tree-dump-times "__cxa_contract_violation_pre_enforce_ex" 11 "original" } }
