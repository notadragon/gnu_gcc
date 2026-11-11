// OPEN BUG (GCC-998 in gnu_gcc open-issues/; deferred, DECISIONS.md E209
// (c): a documented limitation).
//
// With -g, what the regions of the analysis evaluations leave in the real
// functions' debug information, though not in their code.  The early
// debug information is made, and the CFG first built and cleaned up, from
// the functions as the front end leaves them, regions included, before
// the proof shadows are made and the regions removed from the real
// functions.  So a type or function only a predicate uses is described:
// a temporary's class and its destructor (f10), a lambda's closure type
// (f11), a function called only in a predicate (f13), the constructor and
// member function of an explicit conversion's type (f15's U, whose
// constructor is only declared; f16's W<int>), at every level; and
// from -Og up the debug markers of the statements in an arm holding a
// region are lost where they are kept without the regions -- the arms of
// an `if' with an `else' (f5), a `switch' case (f8), a loop body (f9, at
// -Og and -O1).  The assembly, debug information included, should be what
// it is at proof level 0, where no region is placed, at -O0 as at -O1 (as
// the code is: p4405-prove-optimize-same-code-O0.C), but is not; the
// XFAILs record that.  The shapes that do not differ:
// p4405-shadow-debug-info-statements.C.  Clang's counterpart of the first
// shapes is CLANG-916 (under -fcontracts-prove-assume; what its shadows
// left, CLANG-915, is fixed there).
//
// Mirror: clang/test/Contracts/p4405-shadow-debug-info-types.cpp and
// OpenBugs/p4405-shadow-debug-info-assume.cpp in the llvm_llvm-project fork
// (f10, f11, f15, f16).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -g -gno-record-gcc-switches" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }

struct T { int n; T (int n) : n (n) {} ~T (); bool ok () const { return n > 0; } };
struct U { int n; U (int n); bool ok () const { return n > 0; } };
template <class X> struct W { X x; W (X x) : x (x) {} bool ok () const { return x > 0; } };
int g (int);

int f5 (int i, int n)
{
  if (i < n)
    contract_assert (n > 0);
  else
    contract_assert (n < 0);
  return 5;
}
int f8 (int i, int n)
{
  switch (i)
    {
    case 1:
      contract_assert (n > 0);
      break;
    default:
      break;
    }
  return 8;
}
int f9 (const int *p, int n) { for (int j = 0; j < n; ++j) contract_assert (p[j] > 0); return 9; }
int f13 (int i, int n) { if (i < n) contract_assert (g (n) > 0); return 13; }
int f10 (int n) { contract_assert (T (n).ok ()); return 10; }
int f11 (int n) { contract_assert ([&] { return n > 0; } ()); return 11; }
int f15 (int n) { contract_assert (U (n).ok ()); return 15; }
int f16 (int n) { if (n > 1) contract_assert (W<int> (n).ok ()); return 16; }

// { dg-final { set src [lindex $args 0] } }
// { dg-final { set base [file rootname [file tail $src]] } }
// { dg-final { upvar 1 tool_flags tool dg-extra-tool-flags extra } }
// { dg-final { set common "$tool $extra" } }
// { dg-final { set runs {shadow {} level0 -fcontracts-prove-optimize=0} } }
// { dg-final { foreach level {-O0 -O1} \{ } }
// { dg-final {   foreach {tag opts} $runs \{ } }
// { dg-final {     set out $base-$tag.s } }
// { dg-final {     set flags "additional_flags=$common $level $opts" } }
// { dg-final {     set msg [g++_target_compile $src $out assembly [list $flags]] } }
// { dg-final {     set text($tag) {} } }
// { dg-final {     if {$msg eq {} && [file exists $out]} { set fd [open $out r]; set text($tag) [read $fd]; close $fd } } }
// { dg-final {     file delete -force $out } }
// { dg-final {   \} } }
// { dg-final {   set name "[testname-for-summary] same assembly with -g at $level as at proof level 0" } }
// { dg-final {   if {$text(shadow) ne {} && $text(shadow) eq $text(level0)} { xpass $name } else { xfail $name } } }
// { dg-final { \} } }
