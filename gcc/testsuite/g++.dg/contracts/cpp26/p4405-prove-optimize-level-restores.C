// D4405's proof optimization level (DECISIONS.md E162-E168, E165, E206),
// Clang's level-restores cases: the proofs are made at the proof level,
// with the flags the program was given, whatever the level and the
// function attributes it is compiled with.  In GCC the sums' facts are
// proven at every proof level from 1, and the counted loops' facts at
// none (GCC's loop optimizers do not bound c), so every build that has
// shadows -- -O1, -O3, -Os, -Oz, -Og, levels 1 and 3, with or without
// unrolling -- gives the same results: summ's own optimize ("Os") does not
// matter (E168 (7)), nor does sumc's cold attribute, which the shadow
// keeps; loop3 keeps its pragma.  A -O0 build proves at the proof level
// too, 2 by default, but makes no shadows at level 0 (`follow' there).
// The dg-final code compiles the file with each set of options and
// compares the evaluations that are not proven, by the tags on their
// lines, with those expected.
//
// Mirror: clang/test/Contracts/p4405-prove-optimize-level-restores.cpp in
// the llvm_llvm-project fork (Clang: sum not proven at level 1, loop2
// proven from level 2 where loops are unrolled).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -O1" }

int sum (const int *a, unsigned n)
{
  int t = 0;
  for (unsigned j = 0; j < n; ++j)
    t += a[j];
  contract_assert (t != 1 || n > 0);	// [sum] { dg-bogus "cannot be proven" }
  return t;
}

[[gnu::optimize ("Os")]] int summ (const int *a, unsigned n)
{
  int t = 0;
  for (unsigned j = 0; j < n; ++j)
    t += a[j];
  contract_assert (t != 1 || n > 0);	// [summ] { dg-bogus "cannot be proven" }
  return t;
}

[[gnu::cold]] int sumc (const int *a, unsigned n)
{
  int t = 0;
  for (unsigned j = 0; j < n; ++j)
    t += a[j];
  contract_assert (t != 1 || n > 0);	// [sumc] { dg-bogus "cannot be proven" }
  return t;
}

bool cond (int);

int loop2 ()
{
  int c = 0;
  for (int i = 0; i < 8; ++i)
    if (cond (i))
      ++c;
  contract_assert (c <= 8);	// [loop2] { dg-error "contract assertion cannot be proven" }
  return c;
}

int loop3 ()
{
  int c = 0;
#pragma GCC unroll 0
  for (int i = 0; i < 8; ++i)
    if (cond (i))
      ++c;
  contract_assert (c <= 8);	// [loop3] { dg-error "contract assertion cannot be proven" }
  return c;
}

// { dg-final { set src [lindex $args 0] } }
// { dg-final { upvar 1 tool_flags tool dg-extra-tool-flags extra } }
// { dg-final { set fd [open $src r] } }
// { dg-final { set lines [split [read $fd] \n] } }
// { dg-final { close $fd } }
// { dg-final { set tags [list] } }
// { dg-final { set n 0 } }
// { dg-final { foreach l $lines { incr n ; if {[regexp {// \[(\w+)\]} $l - t]} { lappend tags $n $t } } } }
// { dg-final { set runs [list] } }
// { dg-final { lappend runs -O3 {loop2 loop3} } }
// { dg-final { lappend runs -Os {loop2 loop3} } }
// { dg-final { lappend runs -Oz {loop2 loop3} } }
// { dg-final { lappend runs -Og {loop2 loop3} } }
// { dg-final { lappend runs {-O1 -fcontracts-prove-optimize=follow} {loop2 loop3} } }
// { dg-final { lappend runs {-O2 -fcontracts-prove-optimize=1} {loop2 loop3} } }
// { dg-final { lappend runs {-O1 -fcontracts-prove-optimize=follow -funroll-loops} {loop2 loop3} } }
// { dg-final { lappend runs {-O1 -fno-unroll-loops} {loop2 loop3} } }
// { dg-final { lappend runs {-O2 -fno-unroll-loops -fcontracts-prove-optimize=3} {loop2 loop3} } }
// { dg-final { lappend runs {-O2 -funroll-loops -fcontracts-prove-optimize=3} {loop2 loop3} } }
// { dg-final { lappend runs -O0 {loop2 loop3} } }
// { dg-final { lappend runs {-O0 -fcontracts-prove-optimize=1} {loop2 loop3} } }
// { dg-final { lappend runs {-O0 -fcontracts-prove-optimize=follow} {sum summ sumc loop2 loop3} } }
// { dg-final { set base [file rootname [file tail $src]] } }
// { dg-final { foreach {opts want} $runs \{ } }
// { dg-final {   set flags "additional_flags=$tool $extra $opts" } }
// { dg-final {   set msg [g++_target_compile $src $base.o object [list $flags]] } }
// { dg-final {   file delete -force $base.o } }
// { dg-final {   set got [list] } }
// { dg-final {   foreach {ln t} $tags { if {[regexp ":$ln:\[0-9\]+: error: contract assertion cannot be proven" $msg]} { lappend got $t } } } }
// { dg-final {   set name "[testname-for-summary] {$opts} not proven: $want" } }
// { dg-final {   if {$got eq $want} { pass $name } else { fail "$name (got: $got)" } } }
// { dg-final { \} } }
