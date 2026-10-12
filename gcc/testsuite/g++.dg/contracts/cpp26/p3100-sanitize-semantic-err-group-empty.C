// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=undefined -fsanitize-semantic=undefined:enforce,all:observe,undefined:quick_enforce" }
int main() { return 0; }
// P3100: a meta-group skips the members that do not take the semantic, but
// when none of them takes it the request would change nothing, so it is an
// error (notadragon_wg21 DECISIONS.md E60).  Every member of "undefined" is
// routed, so none takes plain enforce or observe, and "all" is routed as a
// whole too; quick_enforce is taken, so that pair is accepted.
//
// Mirror: clang/test/Contracts/p3100-sanitize-semantic-err-group-empty.cpp in
// the llvm_llvm-project fork.
// { dg-error ".-fsanitize-semantic=undefined:enforce. is not supported: no members of .undefined. support .enforce." "" { target *-*-* } 0 }
// { dg-message "use .noexcept_enforce. \\(with .-fcontracts-p4298.\\) or .quick_enforce." "" { target *-*-* } 0 }
// { dg-error ".-fsanitize-semantic=all:observe. is not supported: no members of .all. support .observe." "" { target *-*-* } 0 }
// { dg-message "use .noexcept_observe. with .-fcontracts-p4298." "" { target *-*-* } 0 }
