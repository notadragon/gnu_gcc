// OPEN BUG (GCC-998 in gnu_gcc open-issues/; deferred, DECISIONS.md E209
// (c): a documented limitation).
//
// With -g, an override shadow that needs a snapshot of a parameter the
// body modifies (B::get's postcondition reads x, which D::get assigns)
// changes the scopes the debug information describes: the front end
// declares the snapshot in a block of its own around the body, so D::get's
// local r is described in a lexical block, where without the override
// shadows it is in the function's outermost scope.  The assembly, debug
// information included, should be what it is at proof level 0, where no
// region is placed, at -O0 as at -O2 (as the code is:
// p4405-override-same-code.C), but is not; the XFAILs record that.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -g -gno-record-gcc-switches" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-analysis-semantic=diagnose -Wno-contract-unproven" }

int sink (int);

struct B { virtual int get (const int x) pre (x > 0) post (r : r >= x); };
struct D : B
{
  int get (int x) override { int r = x; x = sink (x); return r + (x > 0); }
};
B *make () { return new D; }

// { dg-final { set src [lindex $args 0] } }
// { dg-final { set base [file rootname [file tail $src]] } }
// { dg-final { upvar 1 tool_flags tool dg-extra-tool-flags extra } }
// { dg-final { set common "$tool $extra" } }
// { dg-final { set runs {shadow {} level0 -fcontracts-prove-optimize=0} } }
// { dg-final { foreach level {-O0 -O2} \{ } }
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
