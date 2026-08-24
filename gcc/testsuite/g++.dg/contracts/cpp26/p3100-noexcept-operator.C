// P3100: an implicit contract assertion must never change the result of the
// noexcept operator, whatever evaluation semantic is selected for it.  The
// implicit UB guards for the front-end expression checks (div/mod by zero,
// signed division overflow, shift, float->int conversion) are injected while
// building the expression, so before the fix they leaked into the noexcept
// operator's tree walk and made noexcept(<guarded expr>) false for every
// checking semantic.  The fix suppresses guard emission in an unevaluated
// operand (cp_unevaluated_operand), so noexcept is unaffected.  Compile-only:
// each static_assert must hold.  The config below deliberately assigns a
// different checking semantic to each check to prove the transparency is
// semantic-independent.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-noexcept-operator.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int    g_arr[3] = { };

// ---- front-end checks: these carried the bug ----
void div_zero (int d)      { static_assert (noexcept (6 / d)); }        // enforce
void mod_zero (int d)      { static_assert (noexcept (6 % d)); }        // enforce
void div_ovf  (int a, int b){ static_assert (noexcept (a / b)); }       // quick_enforce
void shl      (int x, int s){ static_assert (noexcept (x << s)); }      // noexcept_enforce
void shr      (int x, int s){ static_assert (noexcept (x >> s)); }      // noexcept_enforce
void fcast    (double f)   { static_assert (noexcept (int (f))); }      // noexcept_observe

// ---- middle-end checks: never affected the noexcept operator, locked in ----
void add_ovf  (int a, int x){ static_assert (noexcept (a + x)); }
void sub_ovf  (int a, int x){ static_assert (noexcept (a - x)); }
void mul_ovf  (int a, int x){ static_assert (noexcept (a * x)); }
void bounds   (int i)      { static_assert (noexcept (g_arr[i])); }
void deref    (int *p)     { static_assert (noexcept (*p)); }

// ---- negative controls: the fix must not over-suppress real throwers ----
int thrower ();                                  // potentially throwing
void nc1 ()                { static_assert (!noexcept (thrower ())); }
void nc2 (int d)           { static_assert (!noexcept (thrower () / d)); }
