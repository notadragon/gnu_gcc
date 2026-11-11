// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=null,return,unreachable -fsanitize-semantic=undefined:noexcept_observe -fsanitize-semantic-print" }
int main() { return 0; }
// GROUP name "undefined" with "noexcept_observe": null, return, and
// unreachable are all routed UBSan checks (full-coverage routing), and the
// routed allowed set includes noexcept_observe uniformly (it is realized
// through the contract-violation handler, independent of a check's native
// can_recover capability -- return/unreachable have can_recover==false yet
// still take it; at run time their noreturn report leg simply terminates after
// the handler).  So the group semantic applies to every member.  P3100 Task
// 1.3 stores it PER MEMBER BIT.  Compiles clean; only these three checks are
// enabled so the debug seam prints exactly these lines.
// { dg-regexp "null: noexcept_observe" }
// { dg-regexp "return: noexcept_observe" }
// { dg-regexp "unreachable: noexcept_observe" }
