// basic.contract.eval/p8
// If a contract violation occurs in a context that is manifestly
// constant-evaluated ([expr.const]), and the evaluation semantic is not
// terminating, then a diagnostic shall be emitted.
//
// intro.compliance.general/2.34
// a contract assertion ([basic.contract.eval]) evaluated with a checking
// semantic in a manifestly constant-evaluated context ([expr.const])
// resulting in a contract violation...
// ... shall issue at least one diagnostic message.
// { dg-do compile { target c++23 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }

consteval void foo( auto x ) pre( false ) {}
// { dg-warning {contract predicate is false in constant expression} "" { target *-*-* } .-1 }
int main() {
  foo( 42 );
}
