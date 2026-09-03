/* [dcl.contract.func]'s restrictions on a parameter odr-used by a
   postcondition -- it must be const, and must not have array or function type
   -- apply to a lambda's call operator like any other function.

   GCC has always applied them.  This is a MIRROR of the Clang test
   clang/test/Contracts/postcondition-param-lambda.cpp, added when Clang was
   found to skip all three for a non-generic lambda: Clang's checks hang off
   its equivalent of the ordinary function-declarator path, which a lambda
   never reaches, while a *generic* lambda was diagnosed anyway by the
   instantiation path.  Nothing pinned the GCC side, so nothing would have
   caught GCC regressing to match.  */

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

int fn_control (int b) post (b > 0) { return b; }  // { dg-error "value parameter used in a postcondition must be const" }

void
non_generic ()
{
  auto non_const = [] (int b) post (b > 0) { return b; };  // { dg-error "value parameter used in a postcondition must be const" }

  /* NOTE the diagnostic GCC picks for these two.  It classifies from the
     DECAYED parameter type, so `const int a[4]' is a non-const `const int *'
     and `int (*f) ()' is a non-const pointer -- both cited as the const rule
     rather than as the array/function rules.  Clang classifies from the
     original type and cites "cannot have an array type" / "a function type".
     Both compilers reject all three; only the reason given differs, so this
     is recorded rather than asserted as a defect either way.  */
  auto array = [] (const int a[4]) post (a[0] > 0) { return a[0]; };  // { dg-error "value parameter used in a postcondition must be const" }

  auto function = [] (int (*f) ()) post (f () > 0) { return f (); };  // { dg-error "value parameter used in a postcondition must be const" }

  /* A reference parameter is exempt -- the rule is about non-reference
     parameters -- and so is a const one.  */
  auto by_ref = [] (int &b) post (b > 0) { return b; };
  auto by_const = [] (const int b) post (b > 0) { return b; };

  /* The result binding alone is always fine.  */
  auto result_only = [] (int b) post (r : r > 0) { return b; };

  /* A precondition may name a non-const value parameter: the rule is a
     postcondition rule.  */
  auto in_pre = [] (int b) pre (b > 0) { return b; };

  (void) non_const; (void) array; (void) function;
  (void) by_ref; (void) by_const; (void) result_only; (void) in_pre;
}

/* A generic lambda: diagnosed when the parameter type is known.  Two
   diagnostics come out here, not one -- the named form from the generic
   declaration and the unnamed form from the instantiated call operator -- so
   both are pinned.  Expecting only one leaves the other as an excess error,
   which is how this was found.  */
void
generic ()
{
  auto g = [] (auto b) post (b > 0) { return b; };  // { dg-error "value parameter .b. used in a postcondition must be const" }
  // { dg-error "a value parameter used in a postcondition must be const" "" { target *-*-* } .-1 }
  g (1);
}

/* A lambda nested inside a lambda.  */
void
nested ()
{
  auto outer = [] {
    auto inner = [] (int b) post (b > 0) { return b; };  // { dg-error "value parameter used in a postcondition must be const" }
    return inner (1);
  };
  (void) outer;
}
