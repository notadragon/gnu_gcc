// P4405 stage 3: the proof shadows change nothing in the output.  Without
// -fcontracts-prove-assume, a translation unit whose evaluations are decided
// in proof shadows (some proven, some not, a predicate-only function, a
// static helper only a caller-side precondition's predicate and the real
// code share, a constructor's two clones, a loop) is compiled to exactly the
// assembly it compiles to without the shadows, and exactly as with every
// evaluation ignore: each shadow is removed before expansion, and the
// markers are gone from the real functions before they are optimized.  The
// dg-final code compiles the file three more times, with this test's own
// options and then without the shadows and with ignore, and compares the
// assembly.
//
// Mirror: clang/test/Contracts/p4405-shadow-same-code.cpp in the
// llvm_llvm-project fork (which compares the object files as well).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }

inline bool in_range (int i, int n) { return i >= 0 && i < n; }
static int helper (int v) { return v * 3 + 1; }

int lookup (const int *a, int i, int n)
  pre (in_range (i, n))
{
  contract_assert (i < n);
  return a[i];
}

static int scaled (int v)
  pre (helper (v) > 0)
{
  return helper (v) * 2;
}

int caller (int v)
{
  if (v > 0)
    return scaled (v);
  return 0;
}

struct B { int b; B (int x) : b (x) {} };
struct D : virtual B
{
  D (int x) : B (x) { contract_assert (x != 0); }
};
D make (int x) { return D (x); }

int loop (const char *s)
{
  int n = 0;
  for (int i = 0; s[i]; ++i)
    {
      contract_assert (i >= 0);
      ++n;
    }
  return n;
}

// { dg-final { set src [lindex $args 0] } }
// { dg-final { set base [file rootname [file tail $src]] } }
// { dg-final { upvar 1 tool_flags tool dg-extra-tool-flags extra } }
// { dg-final { set common "$tool $extra" } }
// { dg-final { set runs {shadow {} noshadow -fno-contracts-prove-shadow ignore -fcontract-evaluation-semantic=ignore} } }
// { dg-final { foreach {tag opts} $runs \{ } }
// { dg-final {   set out $base-$tag.s } }
// { dg-final {   set flags "additional_flags=$common $opts" } }
// { dg-final {   set msg [g++_target_compile $src $out assembly [list $flags]] } }
// { dg-final {   set text($tag) {} } }
// { dg-final {   if {$msg eq {} && [file exists $out]} { set fd [open $out r]; set text($tag) [read $fd]; close $fd } } }
// { dg-final {   file delete -force $out } }
// { dg-final { \} } }
// { dg-final { set name "[testname-for-summary] same assembly" } }
// { dg-final { if {$text(shadow) ne {} && $text(shadow) eq $text(noshadow)} { pass "$name as without the shadows" } else { fail "$name as without the shadows" } } }
// { dg-final { if {$text(shadow) ne {} && $text(shadow) eq $text(ignore)} { pass "$name as with ignore" } else { fail "$name as with ignore" } } }
