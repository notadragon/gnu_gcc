// P4405 stage 3, -fcontracts-prove-shadow (implied by -fcontracts-p4405):
// at -O2 a prove evaluation is proven when the optimizers can show, in the
// function's proof shadow, that its predicate cannot be false where it is
// evaluated -- a dominating condition, a value propagated through an
// inlined helper, the function's own arithmetic -- and diagnosed otherwise.
// P4021R2 Example 2 (an index reaching the array size) stays ill-formed --
// and is not "always false", since it holds for the other iterations; GCC
// warns about the loop itself whatever the semantic.
// Preconditions at each call (D4405), postconditions and assertions in
// the function body.  A predicate
// the shadow shows false whenever it is evaluated is "always false" (here
// one folding cannot decide: it does not read a local variable's value).
//
// Mirror: clang/test/Contracts/p4405-shadow-prove.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=prove" }

static int clamp (int v) { return v < 0 ? 0 : v > 9 ? 9 : v; }

int guarded (const int *a, int i, int n)
  pre (n >= 0)				// { dg-message "the predicate of the contract assertion" }
{
  if (i >= 0 && i < n)
    {
      contract_assert (i < n);		// { dg-bogus "cannot be proven|always false" }
      contract_assert (i >= 0);		// { dg-bogus "cannot be proven|always false" }
      return a[i];
    }
  contract_assert (i < n);		// { dg-error "contract assertion cannot be proven" }
  return 0;
}

int call_guarded (const int *a, int i, int n)
{
  int r = guarded (a, i, 4);		// { dg-bogus "cannot be proven|always false" }
  if (n > 0)
    r += guarded (a, i, n);		// { dg-bogus "cannot be proven|always false" }
  return r + guarded (a, i, n);		// { dg-error "contract assertion cannot be proven" }
}

int propagated (int v)
{
  int c = clamp (v);
  contract_assert (c >= 0 && c <= 9);	// { dg-bogus "cannot be proven|always false" }
  contract_assert (c < 9);		// { dg-error "contract assertion cannot be proven" }
  return c;
}

int result (int x)
  post (r: r > 0)			// { dg-bogus "cannot be proven|always false" }
  post (r: r > 1)			// { dg-error "contract assertion cannot be proven" }
{
  return x > 0 ? x : 1;
}

// P4021R2 Example 2.
int example2 ()
{
  const int buf_size = 4;
  char buf[buf_size];
  for (int i = 0; i != 5; ++i)		// { dg-message "within this loop" }
    {
      contract_assert (i < buf_size);	// { dg-error "contract assertion cannot be proven" }
      buf[i] = 3;			// { dg-warning "iteration 4 invokes undefined behavior" }
    }
  return buf[0];
}

int bounded ()
{
  const int buf_size = 4;
  char buf[buf_size];
  for (int i = 0; i != 4; ++i)
    {
      contract_assert (i < buf_size);	// { dg-bogus "cannot be proven|always false" }
      buf[i] = 4;
    }
  return buf[0];
}

int always_false (int y)
{
  int k = 5;
  contract_assert (k != 5);		// { dg-error "contract predicate is always false" }
  return k + y;
}
