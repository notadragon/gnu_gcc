// D4405's proof optimization level (DECISIONS.md E162-E168, E206): at a
// fixed proof level the real code is the same, whatever the level it is
// generated at, as without the shadows and with every evaluation ignore.
// The proof shadows are optimized at level 2 and call level copies of
// their callees where those are at another level (-O0, -O1, -Og, -O3,
// -Os, -Oz); neither the shadows nor the copies take a clone number or a
// function number.  The shape is p4405-shadow-same-code-numbering.C's (a
// split part's name and label numbers); the dg-final code compiles it at
// each level with the shadows, without them and with ignore, and compares
// the assembly, which holds the label numbers.
//
// Mirror: clang/test/Contracts/p4405-prove-optimize-same-code.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2 -fcontracts-prove-optimize=2" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }

#include "p4405-shadow-same-code-numbering.C"

// { dg-final { set src [lindex $args 0] } }
// { dg-final { set base [file rootname [file tail $src]] } }
// { dg-final { upvar 1 tool_flags tool dg-extra-tool-flags extra } }
// { dg-final { set common "$tool $extra" } }
// { dg-final { set runs {shadow {} noshadow -fno-contracts-prove-shadow ignore -fcontract-evaluation-semantic=ignore} } }
// { dg-final { foreach level {-O0 -O1 -Og -O2 -O3 -Os -Oz} \{ } }
// { dg-final {   foreach {tag opts} $runs \{ } }
// { dg-final {     set out $base-$tag.s } }
// { dg-final {     set flags "additional_flags=$common $level $opts" } }
// { dg-final {     set msg [g++_target_compile $src $out assembly [list $flags]] } }
// { dg-final {     set text($tag) {} } }
// { dg-final {     if {$msg eq {} && [file exists $out]} { set fd [open $out r]; set text($tag) [read $fd]; close $fd } } }
// { dg-final {     file delete -force $out } }
// { dg-final {   \} } }
// { dg-final {   set name "[testname-for-summary] same assembly at $level" } }
// { dg-final {   if {$text(shadow) ne {} && $text(shadow) eq $text(noshadow)} { pass "$name as without the shadows" } else { fail "$name as without the shadows" } } }
// { dg-final {   if {$text(shadow) ne {} && $text(shadow) eq $text(ignore)} { pass "$name as with ignore" } else { fail "$name as with ignore" } } }
// { dg-final { \} } }
