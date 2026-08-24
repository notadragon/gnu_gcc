// P3100 x P4298: a THROWING contract-violation handler under the non-throwing
// evaluation semantics noexcept_enforce / noexcept_observe must TERMINATE the
// program for every built-in implicit check -- the exception must never escape
// (P4298: these semantics are non-throwing; p3100-check-table.md "confirm the
// non-throwing variants ... do NOT propagate").
//
// This file covers the front-end-synthesized checks (all 7 semantics) and the
// three middle-end checks (noexcept_ supported, throwing enforce/observe
// clamped), under BOTH noexcept_enforce (namespace nx_enf) and noexcept_observe
// (namespace nx_obs), selected per-namespace by the configuration file.
//
// Each check is triggered in a forked child that installs a set_terminate
// marker exiting 77; the child wraps the trigger in try/catch so that an
// exception which escapes the barrier (the bug) is caught and exits 1 instead.
// The parent asserts every child exited via the terminate marker (77): that
// distinguishes a correct terminate from either an escaped-and-caught exception
// or a raw crash.  dg-do run passing (exit 0) is the assertion.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0 -g -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-noexcept-throw-terminate-native.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <climits>
#include <sys/wait.h>
#include <unistd.h>

struct E {};
void handle_contract_violation (const std::contracts::contract_violation&)
{
  throw E{};                     // must never escape under a noexcept_ semantic
}

// Runtime-fed operands defeat constant folding (a constant-expression trigger
// would be a compile error) so every check is a real run-time violation.
static volatile int vz = 0, vbig = 100, vimin = INT_MIN, vm1 = -1, vmax = INT_MAX;
static volatile double vhuge = 1e300;
enum E3 { A0, A1, A2 };          // valid range 0..3

#define CHECKS								\
  C (shift,    return 1 << vbig;)          /* expr.shift.neg.and.width      */ \
  C (divzero,  return 7 / vz;)             /* expr.mul.div.by.zero.int (/)  */ \
  C (remzero,  return 7 % vz;)             /* expr.mul.div.by.zero.int (%)  */ \
  C (divovf,   return vimin / vm1;)        /* expr.mul.representable...      */ \
  C (fpcast,   return (int) vhuge;)        /* conv.fpint.float.not.repr.     */ \
  C (bounds,   int a[4] = {}; return a[vbig];) /* expr.add.out.of.bounds.known */ \
  C (flowoff,  if (vz > 999999) return 1;) /* stmt.return.flow.off          */ \
  C (overflow, return vmax + vbig;)        /* expr.expr.eval.signed.integer  (mid-end) */ \
  C (enumload, E3 e; *(int*)&e = 7; return e ? 1 : 0;) /* conv.lval...bool.enum (mid-end) */ \
  C (nullderef,int* p = (int*)(long)vz; return *p;)    /* expr.unary.dereference.nullptr (mid-end) */

#define C(name, body) __attribute__((noinline)) static int name () { body }
namespace nx_enf { CHECKS }
#undef C
#define C(name, body) __attribute__((noinline)) static int name () { body }
namespace nx_obs { CHECKS }
#undef C

typedef int (*Fn) ();
static int run_terminates (const char* label, Fn fn)
{
  pid_t pid = fork ();
  if (pid == 0)
    {
      std::set_terminate ([] { std::_Exit (77); });   // correct: terminate
      try { (void) fn (); } catch (...) { std::_Exit (1); }  // bug: escaped
      std::_Exit (2);                                  // bug: check never fired
    }
  int st = 0;
  waitpid (pid, &st, 0);
  bool ok = WIFEXITED (st) && WEXITSTATUS (st) == 77;
  if (!ok)
    std::printf ("FAIL %-22s exited=%d code=%d signalled=%d sig=%d\n", label,
		 WIFEXITED (st), WIFEXITED (st) ? WEXITSTATUS (st) : -1,
		 WIFSIGNALED (st), WIFSIGNALED (st) ? WTERMSIG (st) : -1);
  return ok ? 0 : 1;
}

int main ()
{
  int bad = 0;
#define C(name, body)							\
  bad |= run_terminates ("noexcept_enforce/" #name, nx_enf::name);	\
  bad |= run_terminates ("noexcept_observe/" #name, nx_obs::name);
  CHECKS
#undef C
  return bad;                    // 0 => every check terminated as required
}
