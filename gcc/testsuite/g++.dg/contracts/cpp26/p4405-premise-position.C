// P4405 premises decided by this TU (T1-GCC): where the assertion is.
// An inline callee's precondition evaluated callee-side or caller-side, its
// postcondition callee-side or caller-side, and a contract_assert in its
// body, all with enforce, the callee always_inline or noinline.  A
// caller-side evaluation is in a wrapper local to this TU, inlined into the
// shadow and the real function alike: decided here even when the callee is
// not inlined, and the callee then holds no evaluation of its own.
//
// Each consumer is a diagnose evaluation: d_* can be proven only from the
// callee's assertion, i_* from a dominating test, so only what makes the
// whole shadow conditional matters there.  -fdump-tree-contract_shadow
// shows each result ("contract obligation N (line L): sound").
//
// Mirror: clang/test/Contracts/p4405-premise-position.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-premise-position.json" }

int sink (int);

namespace calleeside_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_pre_callee_always (int x)
{
  calleeside_always::h (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_pre_callee_always (int x, int y)
{
  calleeside_always::h (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

namespace callerside_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_pre_caller_always (int x)
{
  callerside_always::h (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_pre_caller_always (int x, int y)
{
  callerside_always::h (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

namespace postcallee_always {
[[gnu::always_inline]] inline int h (int x) post (r: r > 0) { return x; }
}

int d_post_callee_always (int x)
{
  int r = postcallee_always::h (x);
  contract_assert (r > 0);	// sound
  return 0;
}

int i_post_callee_always (int x, int y)
{
  postcallee_always::h (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

namespace postcaller_always {
[[gnu::always_inline]] inline int h (int x) post (r: r > 0) { return x; }
}

int d_post_caller_always (int x)
{
  int r = postcaller_always::h (x);
  contract_assert (r > 0);	// sound
  return 0;
}

int i_post_caller_always (int x, int y)
{
  postcaller_always::h (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

namespace body_always {
[[gnu::always_inline]] inline void h (int x)  { contract_assert (x > 0); sink (x); }
}

int d_assert_always (int x)
{
  body_always::h (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_assert_always (int x, int y)
{
  body_always::h (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

namespace calleeside_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_pre_callee_never (int x)
{
  calleeside_never::h (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_pre_callee_never (int x, int y)
{
  calleeside_never::h (y);
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

namespace callerside_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_pre_caller_never (int x)
{
  callerside_never::h (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_pre_caller_never (int x, int y)
{
  callerside_never::h (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

namespace postcallee_never {
[[gnu::noinline]] inline int h (int x) post (r: r > 0) { return x; }
}

int d_post_callee_never (int x)
{
  int r = postcallee_never::h (x);
  contract_assert (r > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_post_callee_never (int x, int y)
{
  postcallee_never::h (y);
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

namespace postcaller_never {
[[gnu::noinline]] inline int h (int x) post (r: r > 0) { return x; }
}

int d_post_caller_never (int x)
{
  int r = postcaller_never::h (x);
  contract_assert (r > 0);	// sound
  return 0;
}

int i_post_caller_never (int x, int y)
{
  postcaller_never::h (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

namespace body_never {
[[gnu::noinline]] inline void h (int x)  { contract_assert (x > 0); sink (x); }
}

int d_assert_never (int x)
{
  body_never::h (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_assert_never (int x, int y)
{
  body_never::h (y);
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

// { dg-final { scan-tree-dump {\(line 29\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 37\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 48\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 56\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 67\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 75\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 86\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 94\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 105\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 113\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 124\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 132\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 143\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 151\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 162\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 170\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 181\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 189\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 200\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 208\): conditional} "contract_shadow" } }
