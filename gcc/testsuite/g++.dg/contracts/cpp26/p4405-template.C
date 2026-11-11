// P4405: a contract of a template is diagnosed once per instantiation; a
// template that is never instantiated is not diagnosed.  D4405: a
// precondition is diagnosed at each call, so here once per instantiation
// of the template that calls the function.
//
// Mirror: clang/test/Contracts/p4405-template.cpp in the llvm_llvm-project
// fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove" }

template<typename T>
T f (T x)
  pre (x > 0)	// { dg-message "the predicate of the contract assertion" }
{ return x; }

template<typename T>
T call (T y)
{
  return f (y);	// { dg-error "contract assertion cannot be proven" }
}

template<typename T>
T never (T x)
  pre (x > 0)	// { dg-bogus "cannot be proven" }
{ return x; }

// { dg-message "T = int" "" { target *-*-* } 0 }
// { dg-message "T = long" "" { target *-*-* } 0 }

template<typename T>
T never_call (T y)
{
  return never (y);	// { dg-bogus "cannot be proven" }
}

int g (int y)
{
  int a = call (y);		// { dg-message "required from here" }
  long b = call ((long) y);	// { dg-message "required from here" }
  return a + (int) b;
}
