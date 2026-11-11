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
//
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -Wno-unused-value" }

struct S
{
  int x;
  bool ok () const;
};

// The report's own case.
void f (bool b) post ((b, true)) {}

// The same through an explicit discard.
void g (bool b) post (((void) b, true)) {}

// ... and with a result-name-introducer, which is a different parse: the
// predicate is dependent while it is being parsed, so the discarded operand
// has not been folded away by the time the check runs.
int h (int x) post (r : ((x, true) && r == r)) { return x; }

// A parameter of class type, discarded.
void cls (S s) post ((s, true)) {}

// Nested commas: only the last operand is the value.
int nested (int x, int y) post (r : ((x, y, true) && r == r)) { return x + y; }

// The parameter is discarded in one place and odr-used in another: the
// odr-use still counts.
int both (int x) post (r : ((x, true) && x == r)) { return x; } // { dg-error "value parameter used in a postcondition must be const" }

// The RIGHT operand of a comma is the value of the expression, so it is
// odr-used.
bool rhs (bool b) post ((true, b)) { return b; } // { dg-error "value parameter used in a postcondition must be const" }

// A discarded CALL still odr-uses its arguments -- the parameter is not a
// potential result of the call, it is an argument to it.
bool use (int);
int arg (int x) post (r : ((use (x), true) && r == r)) { return x; } // { dg-error "value parameter used in a postcondition must be const" }

// Likewise a discarded member call on the parameter.
void memcall (S s) post ((s.ok (), true)) {} // { dg-error "value parameter used in a postcondition must be const" }

// An unevaluated operand is not an odr-use either, and never was.
void unevaluated (int x) post (sizeof (x) > 0) {}

// A reference parameter is outside the rule entirely, discarded or not.
void ref (int &r) post ((r, true)) {}

// CONTROL: the ordinary odr-use still fires.
int plain (int x) post (r : x == r) { return x; } // { dg-error "value parameter used in a postcondition must be const" }

// CONTROL: and is satisfied by const.
int constant (const int x) post (r : x == r) { return x; }

// A LAMBDA in the predicate has parameters of its own, and they are not
// parameters of the enclosing function -- [dcl.contract.func]/7 says "a
// non-reference parameter of f".  GCC used to reject these.
int lambda_param (int x) post ([] (int y) { return y > 0; } (1)) { return x; }

int lambda_param_capture (int x)
    post ([] (int y) { return y > 0; } (1) && x == x) // { dg-error "value parameter used in a postcondition must be const" }
{
  return x;
}

// A contract on the lambda itself still constrains the lambda's own
// parameters.
auto on_lambda = [] (int y) post (r : y == r) { return y; }; // { dg-error "value parameter used in a postcondition must be const" }
