// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=quick_enforce -fsyntax-only" }
// Generated from a contract-feature cross-product specification.
//
// Contract assertions met during a notional or manifestly constant evaluation.
//
// A contract assertion met while evaluating an initializer, an array
// bound or a required constant is evaluated, but what is reported depends on the
// rest of the evaluation.  A false predicate, or one that is not a core constant
// expression, is recorded and evaluation continues.  If the evaluation outside
// the predicates then completes as a constant, each recorded problem is a
// contract violation ([basic.contract.eval]): an error under a terminating
// semantic, a warning under observe, reported once at its own assertion.  If it
// does not complete, everything recorded is discarded -- the variable is
// initialized at run time (an array bound becomes a VLA), or a required constant
// is simply not constant -- and the predicates are evaluated at run time.
//
// Each cell sequences two assertions, each true, false, never constant, constant
// only for a known argument, or reading the argument, against a body that is
// constant, reads the argument, or is never constant, in contexts that decide
// by notional evaluation, that require a constant, or that are not manifestly
// constant-evaluated at all.  The run cells check the run-time half: where the
// notional evaluation fails, the violations are still reported, once each.
//
// Mirror: clang/test/Contracts/matrix-notional-quick-enforce-errors.cpp

bool rt ();

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a const local, under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace const_local_pp_F_NC_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (false)  // { dg-error "contract predicate is false in constant expression" }
  pre (rt ())  // { dg-error "contract condition is not constant" }
{
  return 1;
}

int
use (int p)
{
  const int k = f (p);
  return k;
}
}  // namespace const_local_pp_F_NC_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a const local, under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace const_local_pp_NR_F_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (v > 0)  // { dg-error "contract condition is not constant" }
  pre (false)  // { dg-error "contract predicate is false in constant expression" }
{
  return 1;
}

int
use (int p)
{
  const int k = f (p);
  return k;
}
}  // namespace const_local_pp_NR_F_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a const local, under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace const_local_pp_NR_NC_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (v > 0)  // { dg-error "contract condition is not constant" }
  pre (rt ())  // { dg-error "contract condition is not constant" }
{
  return 1;
}

int
use (int p)
{
  const int k = f (p);
  return k;
}
}  // namespace const_local_pp_NR_NC_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_F_NC_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (false)  // { dg-error "contract predicate is false in constant expression" }
  pre (rt ())  // { dg-error "contract condition is not constant" }
{
  return 1;
}

int g = 1;
int k = f (g);
}  // namespace ns_nonconst_pp_F_NC_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_NR_F_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (v > 0)  // { dg-error "contract condition is not constant" }
  pre (false)  // { dg-error "contract predicate is false in constant expression" }
{
  return 1;
}

int g = 1;
int k = f (g);
}  // namespace ns_nonconst_pp_NR_F_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a namespace-scope variable (constant or dynamic initialization), under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace ns_nonconst_pp_NR_NC_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (v > 0)  // { dg-error "contract condition is not constant" }
  pre (rt ())  // { dg-error "contract condition is not constant" }
{
  return 1;
}

int g = 1;
int k = f (g);
}  // namespace ns_nonconst_pp_NR_NC_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_F_NC_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (false)  // { dg-error "contract predicate is false in constant expression" }
  pre (rt ())  // { dg-error "contract condition is not constant" }
{
  return 1;
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_pp_F_NC_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_NR_F_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (v > 0)  // { dg-error "contract condition is not constant" }
  pre (false)  // { dg-error "contract predicate is false in constant expression" }
{
  return 1;
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_pp_NR_F_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in a constexpr variable, under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace constexpr_var_pp_NR_NC_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (v > 0)  // { dg-error "contract condition is not constant" }
  pre (rt ())  // { dg-error "contract condition is not constant" }
{
  return 1;
}

int g = 1;
constexpr int k = f (g);
}  // namespace constexpr_var_pp_NR_NC_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in an array bound, where VLAs are available, under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace vla_pp_F_NC_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (false)  // { dg-error "contract predicate is false in constant expression" }
  pre (rt ())  // { dg-error "contract condition is not constant" }
{
  return 1;
}

int
use (int p)
{
  char a[f (p)];  // { dg-error "forbids variable length array" }
  return sizeof a;
}
}  // namespace vla_pp_F_NC_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second false; the body is constant.  Called with an object whose value is not known at compile time, in an array bound, where VLAs are available, under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace vla_pp_NR_F_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (v > 0)  // { dg-error "contract condition is not constant" }
  pre (false)  // { dg-error "contract predicate is false in constant expression" }
{
  return 1;
}

int
use (int p)
{
  char a[f (p)];  // { dg-error "forbids variable length array" }
  return sizeof a;
}
}  // namespace vla_pp_NR_F_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first not constant when the argument is unknown (reads it), the second never constant (calls a non-constexpr function); the body is constant.  Called with an object whose value is not known at compile time, in an array bound, where VLAs are available, under quick_enforce.
// The evaluation completes, so the first and the second are each reported, as an error.
// --------------------------------------------------------------------------
namespace vla_pp_NR_NC_u_const_quick_enforce {
constexpr int
f (const int &v)
  pre (v > 0)  // { dg-error "contract condition is not constant" }
  pre (rt ())  // { dg-error "contract condition is not constant" }
{
  return 1;
}

int
use (int p)
{
  char a[f (p)];  // { dg-error "forbids variable length array" }
  return sizeof a;
}
}  // namespace vla_pp_NR_NC_u_const_quick_enforce

// --------------------------------------------------------------------------
// Two preconditions: the first false, the second never constant (calls a non-constexpr function); the body reads the argument.  Called with an object whose value is not known at compile time, in an array bound, where VLAs are available, under quick_enforce.
// The evaluation outside the predicates does not complete as a constant, so nothing about the contracts is reported, and the value is initialized at run time.
// --------------------------------------------------------------------------
namespace vla_pp_F_NC_u_reads_quick_enforce {
constexpr int
f (const int &v)
  pre (false)
  pre (rt ())
{
  return v;
}

int
use (int p)
{
  char a[f (p)];  // { dg-error "forbids variable length array" }
  return sizeof a;
}
}  // namespace vla_pp_F_NC_u_reads_quick_enforce
