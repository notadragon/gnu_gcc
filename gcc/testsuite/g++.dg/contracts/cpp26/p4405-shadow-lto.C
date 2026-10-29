// P4405 stage 3: under -flto the proof shadow is diagnosed at the compile
// step, after the early optimizations (the late ones run at link time), and
// nothing of it reaches the LTO stream.  A dominating condition is proven
// there; an unguarded evaluation is diagnosed.
//
// Mirror: clang/test/Contracts/p4405-shadow-lto.cpp in the llvm_llvm-project
// fork.
// { dg-do compile { target c++26 } }
// { dg-require-effective-target lto }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2 -flto -fno-fat-lto-objects" }
// { dg-additional-options "-fcontract-evaluation-semantic=prove -fmax-errors=1" }

int f (int i, int n)
{
  if (i < n)
    contract_assert (i < n);	// { dg-bogus "cannot be proven|always false" }
  contract_assert (i < n);	// { dg-error "contract assertion cannot be proven" }
  return i;
}
