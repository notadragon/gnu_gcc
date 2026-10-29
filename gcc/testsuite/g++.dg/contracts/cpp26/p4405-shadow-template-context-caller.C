// P4405 stage 3: a caller-side evaluation of a precondition in a template
// instantiation, diagnosed at the end of the compilation by the caller's
// proof shadow, carries the instantiation backtrace of the caller, as at
// -O0 where it is diagnosed at once.  Proven where the instantiation tests
// the argument.
//
// Mirror: clang/test/Contracts/p4405-shadow-template-context-caller.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-shadow-caller-side.json" }

void take (const char *p)
  pre (p)			// { dg-message "the predicate of the contract assertion" }
{
}

template <class T>
void use (T p)
{
  if (p)
    take (p);			// { dg-bogus "cannot be proven|always false" }
  take (p);			// { dg-error "contract assertion cannot be proven" }
}

void call (const char *p)
{
  use (p);			// { dg-message "required from here" }
}

// { dg-message "In instantiation of 'void use\\(T\\) \\\[with T = const char\\*\\\]'" "" { target *-*-* } 0 }
