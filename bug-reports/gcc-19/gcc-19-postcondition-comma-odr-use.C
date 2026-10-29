// GCC-19 / PR126897: a parameter named as the discarded left operand of a
// comma is wrongly treated as odr-used in a postcondition.
//
// Extracted from the fix commit's test,
// gcc/testsuite/g++.dg/contracts/cpp26/pr126897.C (added by gnu_gcc commit
// f3ff6a8e22f), with DejaGnu directives stripped.  Requires
// -fcontracts -Wno-unused-value (C++26).  The `// expected-error` comments
// below mark lines that are *supposed* to still diagnose (they exercise
// genuine odr-uses); everything else is expected to compile cleanly once the
// bug is fixed.  On the unpatched compiler, the lines marked "wrongly
// rejected on unpatched GCC" also produce the diagnostic, which is the bug.

// PR126897: a parameter named as the discarded left operand of a comma is not
// odr-used, so [dcl.contract.func]/7 does not reach it.
//
// [basic.def.odr]/5: a variable x named by a potentially-evaluated expression
// E is odr-used "unless ... x is a variable of non-reference type, and E is an
// element of the set of potential results of a discarded-value expression to
// which the lvalue-to-rvalue conversion is not applied".  [expr.context]
// applies that conversion to a discarded-value expression only when it is a
// glvalue of volatile-qualified type, so for an ordinary parameter it is not
// applied and the naming is not an odr-use.
//
// The check therefore cannot run at id-expression time, where the parser has
// an identifier and not the expression around it; it runs over the finished
// predicate.  These are the cases that distinguishes.

struct S
{
  int x;
  bool ok () const;
};

// The report's own case.  Wrongly rejected on unpatched GCC.
void f (bool b) post ((b, true)) {}

// The same through an explicit discard.  Wrongly rejected on unpatched GCC.
void g (bool b) post (((void) b, true)) {}

// ... and with a result-name-introducer, which is a different parse: the
// predicate is dependent while it is being parsed, so the discarded operand
// has not been folded away by the time the check runs.  Wrongly rejected on
// unpatched GCC.
int h (int x) post (r : ((x, true) && r == r)) { return x; }

// A parameter of class type, discarded.  Wrongly rejected on unpatched GCC.
void cls (S s) post ((s, true)) {}

// Nested commas: only the last operand is the value.  Wrongly rejected on
// unpatched GCC.
int nested (int x, int y) post (r : ((x, y, true) && r == r)) { return x + y; }

// The parameter is discarded in one place and odr-used in another: the
// odr-use still counts.  Genuine odr-use -- expected-error either way.
int both (int x) post (r : ((x, true) && x == r)) { return x; } // expected-error "value parameter used in a postcondition must be const"

// The RIGHT operand of a comma is the value of the expression, so it is
// odr-used.  Genuine odr-use -- expected-error either way.
bool rhs (bool b) post ((true, b)) { return b; } // expected-error "value parameter used in a postcondition must be const"

// A discarded CALL still odr-uses its arguments -- the parameter is not a
// potential result of the call, it is an argument to it.  Genuine odr-use --
// expected-error either way.
bool use (int);
int arg (int x) post (r : ((use (x), true) && r == r)) { return x; } // expected-error "value parameter used in a postcondition must be const"

// Likewise a discarded member call on the parameter.  Genuine odr-use --
// expected-error either way.
void memcall (S s) post ((s.ok (), true)) {} // expected-error "value parameter used in a postcondition must be const"

// An unevaluated operand is not an odr-use either, and never was.
void unevaluated (int x) post (sizeof (x) > 0) {}

// A reference parameter is outside the rule entirely, discarded or not.
void ref (int &r) post ((r, true)) {}

// CONTROL: the ordinary odr-use still fires.  Genuine odr-use --
// expected-error either way.
int plain (int x) post (r : x == r) { return x; } // expected-error "value parameter used in a postcondition must be const"

// CONTROL: and is satisfied by const.
int constant (const int x) post (r : x == r) { return x; }

// A LAMBDA in the predicate has parameters of its own, and they are not
// parameters of the enclosing function -- [dcl.contract.func]/7 says "a
// non-reference parameter of f".  GCC used to reject these.
int lambda_param (int x) post ([] (int y) { return y > 0; } (1)) { return x; }

int lambda_param_capture (int x)
    post ([] (int y) { return y > 0; } (1) && x == x) // expected-error "value parameter used in a postcondition must be const"
{
  return x;
}

// A contract on the lambda itself still constrains the lambda's own
// parameters.  Genuine odr-use -- expected-error either way.
auto on_lambda = [] (int y) post (r : y == r) { return y; }; // expected-error "value parameter used in a postcondition must be const"
