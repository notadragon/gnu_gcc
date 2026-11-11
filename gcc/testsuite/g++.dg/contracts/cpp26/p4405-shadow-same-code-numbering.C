// P4405 stage 3, GCC-907: a proof shadow takes no clone number of its
// function's name and no function number, so the clones made later are named
// and numbered as without it: f, which holds an unproven evaluation, is split,
// and its part is f.part.0, with the label numbers it has without the shadow.
// The assembly is exactly what it is without the shadows and with every
// evaluation ignore; the assembly, not the object code, as the label numbers
// are not in the object.  The dg-final code compiles the file three more times,
// with this test's own options and then without the shadows and with ignore,
// and compares the assembly.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }

extern int sink (int);
extern int cold_path (int);

int f (int n)
{
  contract_assert (n > 0);
  if (n > 1000)
    {
      int s = 0;
      for (int i = 0; i < n; ++i)
	s += sink (i) * sink (i + 1) + cold_path (s);
      for (int i = 0; i < n; ++i)
	s ^= sink (s + i);
      return s;
    }
  return n + 1;
}

int k1 (int n) { return f (n) + f (n + 1); }
int k2 (int n) { return f (n * 2); }

// { dg-final { scan-assembler "_Z1fi\\.part\\.0" } }
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
