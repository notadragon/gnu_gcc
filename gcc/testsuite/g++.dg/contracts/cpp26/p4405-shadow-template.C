// P4405 stage 3: each template instantiation has its own proof shadow, so
// an evaluation is proven or diagnosed per instantiation.  An inline
// function that is used is proven in its shadow; one that is never used is
// never compiled, so it has no shadow and its evaluations are diagnosed as
// without one (GCC diagnoses an unused inline function, DECISIONS.md
// E130), and an uninstantiated template not at all.  -fmax-errors=2
// makes an error for f<5> fail the test.  The diagnostic of f<6>, issued
// at the end of the compilation, carries its instantiation backtrace.
//
// Mirror: clang/test/Contracts/p4405-shadow-template.cpp in the
// llvm_llvm-project fork, except the unused inline function (E130).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=prove -fmax-errors=2" }

template <int N>
int f (int x)
{
  if (x < 10)
    contract_assert (x + N < 15);	// { dg-error "contract assertion cannot be proven" }
  return x;
}

template int f<5> (int);	// proven: x + 5 < 15
template int f<6> (int);	// diagnosed: x = 9 gives 15
// { dg-message "required from here" "" { target *-*-* } .-1 }
// { dg-message "In instantiation of 'int f\\(int\\) \\\[with int N = 6\\\]'" "" { target *-*-* } 0 }
// { dg-bogus "N = 5" "" { target *-*-* } 0 }

template <int N>
int never (int x)
{
  contract_assert (x > N);		// { dg-bogus "cannot be proven|always false" }
  return x;
}

inline int used (int x)
{
  if (x > 0)
    contract_assert (x != 0);		// { dg-bogus "cannot be proven|always false" }
  return x;
}

inline int unused (int x)
{
  if (x > 0)
    contract_assert (x != 0);		// { dg-error "contract assertion cannot be proven" }
  return x;
}

int call_used (int x) { return used (x); }
