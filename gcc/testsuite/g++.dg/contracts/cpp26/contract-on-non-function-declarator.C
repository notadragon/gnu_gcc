// A function-contract-specifier-seq is part of a function DECLARATOR
// ([dcl.contract.func]/1), so writing one on a declarator that does not
// declare a function must be diagnosed.
//
// Every shape below used to be ACCEPTED and the contract then silently
// dropped -- no diagnostic, and no check at any call.  That is the one
// failure mode a contract facility must not have: the user writes a
// precondition, is told nothing, and gets nothing.
//
// The parser cannot apply this rule.  It parses a contract after any
// parameter list, where it can see neither the decl-specifiers (so not
// `typedef`) nor whether it is inside a parameter-declaration-clause, and it
// builds declarators inside-out so an enclosing pointer declarator has not
// been reached yet.  grokdeclarator knows all three, and the requires-clause
// -- which travels in the same declarator slot -- is already refused at the
// same points.
//
// The parameter row is the one worth keeping an eye on.  The check that
// should have caught it, `!FUNC_OR_METHOD_TYPE_P (type)`, is ordered BEFORE
// the function-to-pointer adjustment that would make its guard true, so a
// parameter still looks like a function there.  Testing decl_context == PARM
// directly is what closes it.
//
// Tracked as GCC-45 (typedef) and GCC-46 (parameter) in this repository's
// bug-reports/; both reproduce on stock g++ 16.1.0 and 16.2.0, so both are
// upstream's rather than this branch's.  The Clang mirror is
// clang/test/Contracts/contract-on-function-typedef.cpp, which covers fewer
// rows: Clang already rejected the parameter and the pointer shapes.
//
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

typedef int FTypedef (int) pre (true);           // { dg-error "cannot appear on a typedef" }

// The post spelling, so a fix that only guards `pre` is still caught.
typedef int FTypedefPost (int) post (r : r > 0); // { dg-error "cannot appear on a typedef" }

// An alias-declaration is the same mistake reached through the type-id
// return rather than the typedef return.
using FAlias = int (int) pre (true);             // { dg-error "cannot appear on a type-id" }

// A parameter of function type is adjusted to a pointer to function, so it
// declares no function.
void takes_fn (int bar () pre (true));           // { dg-error "cannot appear on a parameter" }
void defines_fn (int bar () pre (true)) { (void) bar; } // { dg-error "cannot appear on a parameter" }

// A pointer to function has a function TYPE but no function DECLARATOR.
int (*gp) (int) pre (true);                      // { dg-error "cannot appear on a declaration of non-function type" }

// The contract sits on the outer parameter list, which is the return type of
// the function actually being declared.
int (*f2 (int)) (int) pre (true);                // { dg-error "cannot appear on a return type" }

struct S {
  typedef int FMember (int) pre (true);          // { dg-error "cannot appear on a typedef" }
  int (*mp) (int) pre (true);                    // { dg-error "cannot appear on a declaration of non-function type" }
};

template <typename T>
struct U {
  typedef T FDependent (T) pre (true);           // { dg-error "cannot appear on a typedef" }
};

template struct U<int>;

// ---------------------------------------------------------------------------
// Controls.  Each of these IS a function declarator and must keep its
// contract -- a rule written too broadly takes these with it, and a member
// function declaration in particular arrives with decl_context == FIELD, the
// same context as the data member above.
// ---------------------------------------------------------------------------

int ok (int x) pre (x > 0);

auto ok_trailing (int x) -> int pre (x > 0);

struct M {
  int d;
  void mf (int x) pre (x > 0);
  void g () const pre (d >= 0);
};

void M::g () const pre (d >= 0) {}

auto ok_lambda = [] (int x) pre (x > 0) { return x; };

template <class T>
T ok_template (T v) pre (v > T{}) { return v; }

template int ok_template<int> (int);
