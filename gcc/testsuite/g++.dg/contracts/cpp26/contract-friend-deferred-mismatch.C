// A contract mismatch between two friend declarations of the same function is
// diagnosed, like every other redeclaration mismatch.
//
// This was a documented limitation (CC-1 / F31) for as long as a deferred
// contract was something only an in-class member had.  Both friend
// declarations' predicates are DEFERRED_PARSE when duplicate_decls merges
// them, and check_redecl_contract skipped matching whenever either side was
// still deferred -- so the mismatch was accepted in silence.
//
// It could not simply be queued and compared after late-parse, because
// duplicate_decls reclaims the second declaration: it calls
// remove_decl_with_fn_contracts_specifiers (newdecl) and then ggc_free
// (newdecl), so nothing walks that contract to parse it.
//
// What closed it was making every function contract deferred, which forced
// the queue to exist: check_redecl_contract now records the two contract
// vectors and the redeclaration's parameter NAMES, and
// flush_deferred_contract_matches parses the reclaimed side against the
// surviving declaration's parameters, bound under the names the
// redeclaration itself wrote so that a renamed parameter resolves
// positionally.  Only a differing parameter COUNT would defeat that, and two
// declarations of one function cannot differ there.
//
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3850" }

struct C
{
  friend int f (int x) pre (x > 0);
  friend int f (int x) pre (x < 0); // { dg-error "mismatched contract condition" }
};

// The parameter names differ between the two declarations.  That is still
// diagnosed: the surviving declaration's parameters are bound under the names
// the redeclaration wrote, so `y < 0' is compared as `x < 0' against
// `x > 0'.  Only a differing parameter COUNT would defeat it, and two
// declarations of one function cannot differ there.
struct D
{
  friend int g (int x) pre (x > 0);
  friend int g (int y) pre (y < 0); // { dg-error "mismatched contract condition" }
};
