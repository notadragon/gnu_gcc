// P4405 stage 3: P4021R2 Example 3 is folding's -- divide (num, 0) is
// always false with or without the proof shadow -- and the shadow does
// not change it; divide (num, d) with a d the optimizers know is proven by
// the shadow, and one with an unknown d is not.
//
// Mirror: clang/test/Contracts/p4405-shadow-example3.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-shadow-caller-side.json" }

// Force calling code to check for divide-by-0.
static int divide (int num, int denominator)
  pre (denominator != 0)	// { dg-message "the predicate of the contract assertion" }
{
  return num / denominator;
}

int main ()
{
  int num = 10;
  int result = divide (num, 0);	// { dg-error "contract predicate is always false" }
  return result;
}

// Not in main: divide (num, 0) makes everything after it undefined, so
// unreachable in the shadow.
int other (int num, int k)
{
  int d = k > 1 ? 2 : 5;
  int result = divide (num, d);	// { dg-bogus "cannot be proven|always false" }
  result += divide (num, k);	// { dg-error "contract assertion cannot be proven" }
  return result;
}
