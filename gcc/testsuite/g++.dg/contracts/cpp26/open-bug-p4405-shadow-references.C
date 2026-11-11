// OPEN BUG (GCC-995 in gnu_gcc open-issues/; deferred, DECISIONS.md E204
// (a): a documented limitation).
//
// A proof shadow's references to variables and functions still count when
// IPA works out their properties for the real code.  bump, which only the
// predicate of f's assertion calls (so only f's shadow), is the only code
// that stores to counter; without the shadow counter is found read-only and
// its loads in the real code fold away.  Only the predicate of h's
// assertion takes the address of g; without the shadow g's address is not
// taken, so IPA-SRA drops g's unused parameter.  The assembly should be what
// it is without the shadows and with every evaluation ignore (as in
// p4405-shadow-same-code.C), but is not; the XFAILs record that.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }

extern int sink (int);

static int counter;
static bool bump () { ++counter; return true; }

int f (int n)
{
  contract_assert (bump () && n > 0);
  return sink (n) + counter;
}

extern bool ok (int (*) (int, int));

__attribute__ ((noinline)) static int g (int x, int) { return sink (x) * 2; }

int h (int n)
{
  contract_assert (ok (&g));
  return g (n, 1) + g (n + 1, 2);
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
// { dg-final { if {$text(shadow) ne {} && $text(shadow) eq $text(noshadow)} { xpass "$name as without the shadows" } else { xfail "$name as without the shadows" } } }
// { dg-final { if {$text(shadow) ne {} && $text(shadow) eq $text(ignore)} { xpass "$name as with ignore" } else { xfail "$name as with ignore" } } }
