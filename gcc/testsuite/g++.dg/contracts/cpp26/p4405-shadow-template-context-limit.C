// P4405 stage 3: the instantiation backtrace of a diagnostic the proof
// shadows issue at the end of the compilation honours
// -ftemplate-backtrace-limit as one issued at once does: with a limit of 3,
// the innermost and the two outermost contexts and a line saying how many
// were skipped.  diagnose: the diagnostic is a warning.
//
// Mirror: clang/test/Contracts/p4405-shadow-template-context-limit.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=diagnose -ftemplate-backtrace-limit=3" }

template <int N> int a (int x) { contract_assert (x > N); return x; }	// { dg-warning "contract assertion cannot be proven" }
template <int N> int b (int x) { return a<N> (x); }	// { dg-message "required from 'int b\\(int\\) \\\[with int N = 1\\\]'" }
template <int N> int c (int x) { return b<N> (x); }	// { dg-message "skipping 2 instantiation contexts" }
template <int N> int d (int x) { return c<N> (x); }
template <int N> int e (int x) { return d<N> (x); }	// { dg-message "required from 'int e\\(int\\) \\\[with int N = 1\\\]'" }
int z (int x) { return e<1> (x); }			// { dg-message "required from here" }

// { dg-message "In instantiation of 'int a\\(int\\) \\\[with int N = 1\\\]'" "" { target *-*-* } 0 }
