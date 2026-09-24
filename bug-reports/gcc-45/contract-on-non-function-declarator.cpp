// A function-contract-specifier on a declarator that does not declare a
// function is accepted and silently dropped.
//
// The function-contract-specifier-seq of an init-declarator "shall not be
// present unless the declarator declares a function" ([dcl.decl.general]/6).
// Each declarator below takes one anyway and then discards it, so no call is
// ever checked against the predicate.  Every one should be rejected.
//
// This is the accepts-invalid symptom.  The rejects-valid symptom is in
// valid-declarator-rejected.cpp, and the two wrong-code symptoms are in
// contract-silently-dropped.cpp and contract-predicate-wrong-scope.cpp; all
// four are the same parse-position defect seen from different sides.
//
// g++ -std=c++26 -fcontracts -fsyntax-only \
//     contract-on-non-function-declarator.cpp
//
// Expected: rejected.  Actual: accepted with no diagnostic at all.

// A typedef declaration.

typedef int FTypedef (int) pre (true);

typedef int FTypedefPost (int) post (r : r > 0);

struct STypedef {
  typedef int FMember (int) pre (true);
};

template <typename T>
struct UTypedef {
  typedef T FDependent (T) pre (true);
};

template struct UTypedef<int>;

// An alias-declaration, which reaches the type-id return rather than the
// typedef return, so a fix aimed at typedef declarations alone leaves it.

using FAlias = int (int) pre (true);

using FAliasPost = int (int) post (r : r > 0);

struct SAlias {
  using FMemberAlias = int (int) pre (true);
};

template <typename T>
using FAliasTemplate = T (T) pre (true);

using FAliasTemplateInt = FAliasTemplate<int>;

using FAliasPointer = int (*) (int) pre (true);

// A parameter of function type, adjusted to a pointer to function
// ([dcl.fct]/5), so it declares no function.  In defines_fn the enclosing
// function is being defined, and the contract is still dropped.

void takes_fn (int bar () pre (true));

void takes_fn_post (int bar () post (r : r > 0));

void takes_fn_args (int bar (int, int) pre (true));

void defines_fn (int bar () pre (true)) { (void) bar; }

struct SParm {
  void mem (int bar () pre (true));
};

template <typename T>
void tmpl (T bar () pre (true)) { (void) bar; }

template void tmpl<int> (int bar ());

// A pointer to function: a function type, but no function declarator.

int (*gp) (int) pre (true);

struct SPointer {
  int (*mp) (int) pre (true);
};

// ---------------------------------------------------------------------------
// How much of a dropped predicate is looked at, which differs by scope.
//
// At namespace scope the predicate is parsed before it is discarded, so a
// meaningless one is still diagnosed.  Uncommenting either of these gives an
// error, which is the correct half of the behaviour:
//
//   typedef int FBadSyntax (int) pre (1 + * / 2);  // expected primary-expression
//   typedef int FBadName   (int) pre (nosuchname); // not declared in this scope
//
// Inside a class being defined the predicate is token-cached for deferred
// parsing, cp_parser_late_contracts is only ever driven for a FUNCTION_DECL,
// and the cache is discarded unparsed.  Nothing inside the parentheses is
// ever looked at: only their balance is checked, by the token-caching scan.
// Every declaration below is accepted with no diagnostic.
// ---------------------------------------------------------------------------

struct SNeverParsed {
  typedef int FSyntax (int) pre (1 + * / 2);
  typedef int FName   (int) pre (nosuchname);
  typedef int FWords  (int) pre (a b c d);
  typedef int FString (int) pre ("not a bool");
  typedef int FStmt   (int) pre (return 7);
  using   UWords = int (int) pre (a b c d);
  int    (*mpWords) (int) pre (a b c d);
};
