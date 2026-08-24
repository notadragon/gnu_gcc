// Documented limitation (CC-1 / F31): a contract mismatch between two friend
// declarations of the same function -- whose contracts are DEFERRED_PARSE at
// the redeclaration-merge point -- is silently accepted rather than diagnosed,
// unlike every non-deferred redeclaration path (which emits "mismatched
// contract condition in declaration").  check_redecl_contract skips matching
// when either side is still deferred (gcc/cp/contracts.cc).
//
// Why this is not simply "queue and compare after late-parse" (investigated
// 2026-08-07): the second friend declaration (newdecl) is discarded by
// duplicate_decls, which calls remove_decl_with_fn_contracts_specifiers(newdecl)
// -- removing its contract_decl_map entry -- BEFORE end-of-class late-parsing.
// So newdecl's deferred contract is dropped and never late-parsed; there is
// nothing left to compare.  A correct fix must either compare the raw deferred
// token streams at the skip point (a textual match that diverges from the
// semantic matcher -- e.g. it would false-positive on pre((x>0)) vs pre(x>0)),
// or preserve newdecl's tokens and late-parse them in newdecl's own parameter
// scope (the two friend decls may use different parameter names, so olddecl's
// scope cannot be reused).  Both are invasive for this degenerate construct, so
// the limitation is retained by design.
//
// This is marked xfail: the expected diagnostic is NOT currently emitted.  A
// future fix that starts diagnosing the mismatch will turn this into an XPASS,
// prompting removal of the xfail.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3850" }

struct C
{
  friend int f (int x) pre (x > 0);
  friend int f (int x) pre (x < 0); // { dg-error "mismatched contract condition" "deferred-friend contract mismatch (CC-1/F31)" { xfail *-*-* } }
};
