// A valid function-contract-specifier is rejected, because g++ looks for it
// after a parameter list rather than after the declarator.
//
// Two distinct failures, one parse-position defect:
//
//   * When the declarator's outermost part is not a parameter list at all --
//     an array or a reference-to-array -- the parser never looks for a
//     contract, and the specifier is reported as a syntax error.
//
//   * When it is a parameter list, but one belonging to the return type, the
//     contract is parsed at a point where the function's own parameters have
//     gone out of scope, so a predicate naming one does not resolve.  The
//     return type's parameter list is unnamed in both rows below; naming it
//     replaces this error with a silent wrong answer, which is in
//     contract-predicate-wrong-scope.cpp.
//
// All four are well-formed: a function-contract-specifier-seq follows the
// complete declarator ([dcl.decl.general]/1), and the function parameter
// scope reaches the end of that init-declarator ([basic.scope.param]/1.1),
// so the predicate may name a parameter.
//
// g++ -std=c++26 -fcontracts -fsyntax-only valid-declarator-rejected.cpp
//
// Expected: accepted.  Actual: four errors.

struct S { };

// ---------------------------------------------------------------------------
// Controls, first: the same four declarator shapes with no contract on them.
// All four are accepted, here and on every release tested, which is what
// establishes that the shapes themselves are spellable and that the errors
// below are caused by the contract and nothing else.
// ---------------------------------------------------------------------------

char (*returns_array_nc (int i)) [3];

char (&returns_array_ref_nc (int i)) [3];

int (*returns_fn_nc (int i)) (int);

int (S::*returns_memfn_nc (int i)) (int);

// ---------------------------------------------------------------------------
// The same shapes, each with a precondition added.
// ---------------------------------------------------------------------------

// The declarator's outermost part is an array, so the parser is not looking
// for a contract here at all.
//
//   error: expected initializer before 'pre'

char (*returns_array (int i)) [3] pre (i > 0);

char (&returns_array_ref (int i)) [3] pre (i > 0);

// The declarator's outermost part is the return type's parameter list.  The
// contract does reach the function -- see contract-silently-dropped.cpp,
// where a parameter-free predicate in this spelling is checked correctly --
// but it is parsed after the function's own parameter scope has been left,
// so `i` does not resolve.
//
//   error: 'i' was not declared in this scope

int (*returns_fn (int i)) (int) pre (i > 0);

int (S::*returns_memfn (int i)) (int) pre (i > 0);

// ---------------------------------------------------------------------------
// Two further controls, accepted today, which a fix must keep that way.
//
// returns_array_trailing is the row that isolates the rule: it is the first
// declaration above, rewritten with a trailing return type.  It works,
// because the type-id `char (*) [3]` contains no parameter list for the
// contract to bind to, so the parse falls through to the outer declarator.
// Change that type-id to one that does contain a parameter list and the
// contract is silently dropped instead.
// ---------------------------------------------------------------------------

auto returns_array_trailing (int i) -> char (*) [3] pre (i > 0);

auto plain (int i) -> int pre (i > 0);

int plainest (int i) pre (i > 0);
