// P4405 stage 3: the diagnostics of the proof shadows are issued at the end
// of the compilation, but each carries the template instantiation backtrace
// that was live when its evaluation was described, exactly as an error
// issued then does without the shadows (-O0): "In instantiation of",
// each "required from" and "required from here".  The same for an
// evaluation folding decides (always false) while shadows are active, whose
// diagnostic waits for theirs.  The backtrace is printed once for
// consecutive diagnostics of one instantiation.  A non-template function's
// diagnostic has "In function" and no backtrace, also right after one that
// has one: a constexpr function template used in a constant expression is
// instantiated there, so its diagnostic comes before plain's, while the
// other instantiations' come after it, at the end of the translation unit,
// where their bodies are instantiated.
//
// Mirror: clang/test/Contracts/p4405-shadow-template-context.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=prove" }

template <int N>
int f (int x)
{
  if (x < 10)
    contract_assert (x + N < 15);	// { dg-error "contract assertion cannot be proven" }
  contract_assert (N < 0);		// { dg-error "contract predicate is always false" }
  return x;
}

template <int N>
int g (int x)
{
  return f<N> (x) + 1;			// { dg-message "required from 'int g\\(int\\) \\\[with int N = 6\\\]'" }
}

int h (int x)
{
  return g<6> (x);			// { dg-message "required from here" }
}

template int f<7> (int);		// { dg-message "required from here" }

// { dg-message "In instantiation of 'int f\\(int\\) \\\[with int N = 6\\\]'" "" { target *-*-* } 0 }
// { dg-message "In instantiation of 'int f\\(int\\) \\\[with int N = 7\\\]'" "" { target *-*-* } 0 }

template <int N>
constexpr int k (int x)
{
  if (x < 10)
    contract_assert (x + N < 15);	// { dg-error "contract assertion cannot be proven" }
  return x;
}

constexpr int kk = k<6> (1);		// { dg-message "required from here" }

// { dg-message "In instantiation of 'constexpr int k\\(int\\) \\\[with int N = 6\\\]'" "" { target *-*-* } 0 }

int plain (int x)
{
  contract_assert (x > 0);		// { dg-error "contract assertion cannot be proven" }
  return x;
}

// { dg-message "In function 'int plain\\(int\\)'" "" { target *-*-* } 0 }
