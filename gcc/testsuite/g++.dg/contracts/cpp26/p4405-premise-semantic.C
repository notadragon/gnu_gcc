// P4405 premises decided by this TU (T1-GCC): the semantic of the
// callee's assertion in this TU.  An inline callee's precondition with each
// of the nine semantics, the callee always_inline or noinline.  enforce,
// noexcept_enforce, quick_enforce and assume leave code the shadow can take
// the predicate from (quick_enforce's trap is emitted inline, so the
// optimizers see it, as they see enforce's noreturn entry point), and
// ignore, observe, noexcept_observe, prove and diagnose leave none -- the
// shadow never uses another function's proof.  prove and diagnose: analyzed
// at each call (D4405); its unproven marker makes the consumer conditional.
//
// Each consumer is a diagnose evaluation: d_* can be proven only from the
// callee's assertion, i_* from a dominating test, so only what makes the
// whole shadow conditional matters there.  -fdump-tree-contract_shadow
// shows each result ("contract obligation N (line L): sound").
//
// Mirror: clang/test/Contracts/p4405-premise-semantic.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontracts-p4298 -fcontracts-allow-assume -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-premise-semantic.json" }

int sink (int);

namespace sem_ignore_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_ignore_always (int x)
{
  sem_ignore_always::h (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_ignore_always (int x, int y)
{
  sem_ignore_always::h (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

namespace sem_ignore_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_ignore_never (int x)
{
  sem_ignore_never::h (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_ignore_never (int x, int y)
{
  sem_ignore_never::h (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

namespace sem_observe_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_observe_always (int x)
{
  sem_observe_always::h (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_observe_always (int x, int y)
{
  sem_observe_always::h (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

namespace sem_observe_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_observe_never (int x)
{
  sem_observe_never::h (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_observe_never (int x, int y)
{
  sem_observe_never::h (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

namespace sem_enforce_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_enforce_always (int x)
{
  sem_enforce_always::h (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_enforce_always (int x, int y)
{
  sem_enforce_always::h (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

namespace sem_enforce_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_enforce_never (int x)
{
  sem_enforce_never::h (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_enforce_never (int x, int y)
{
  sem_enforce_never::h (y);
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

namespace sem_quick_enforce_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_quick_enforce_always (int x)
{
  sem_quick_enforce_always::h (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_quick_enforce_always (int x, int y)
{
  sem_quick_enforce_always::h (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

namespace sem_quick_enforce_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_quick_enforce_never (int x)
{
  sem_quick_enforce_never::h (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_quick_enforce_never (int x, int y)
{
  sem_quick_enforce_never::h (y);
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

namespace sem_assume_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_assume_always (int x)
{
  sem_assume_always::h (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_assume_always (int x, int y)
{
  sem_assume_always::h (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

namespace sem_assume_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_assume_never (int x)
{
  sem_assume_never::h (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_assume_never (int x, int y)
{
  sem_assume_never::h (y);
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

namespace sem_noexcept_enforce_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_noexcept_enforce_always (int x)
{
  sem_noexcept_enforce_always::h (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_noexcept_enforce_always (int x, int y)
{
  sem_noexcept_enforce_always::h (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

namespace sem_noexcept_enforce_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_noexcept_enforce_never (int x)
{
  sem_noexcept_enforce_never::h (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_noexcept_enforce_never (int x, int y)
{
  sem_noexcept_enforce_never::h (y);
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

namespace sem_noexcept_observe_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_noexcept_observe_always (int x)
{
  sem_noexcept_observe_always::h (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_noexcept_observe_always (int x, int y)
{
  sem_noexcept_observe_always::h (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

namespace sem_noexcept_observe_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_noexcept_observe_never (int x)
{
  sem_noexcept_observe_never::h (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_noexcept_observe_never (int x, int y)
{
  sem_noexcept_observe_never::h (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

namespace sem_prove_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }	// { dg-message "the predicate of the contract assertion" }
}

int d_prove_always (int x)
{
  sem_prove_always::h (x);	// { dg-error "contract assertion cannot be proven" }
  contract_assert (x > 0);	// conditional
  return 0;
}

int i_prove_always (int x, int y)
{
  sem_prove_always::h (y);	// { dg-error "contract assertion cannot be proven" }
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

namespace sem_prove_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }	// { dg-message "the predicate of the contract assertion" }
}

int d_prove_never (int x)
{
  sem_prove_never::h (x);	// { dg-error "contract assertion cannot be proven" }
  contract_assert (x > 0);	// conditional
  return 0;
}

int i_prove_never (int x, int y)
{
  sem_prove_never::h (y);	// { dg-error "contract assertion cannot be proven" }
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

namespace sem_diagnose_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }	// { dg-message "the predicate of the contract assertion" }
}

int d_diagnose_always (int x)
{
  sem_diagnose_always::h (x);	// { dg-warning "contract assertion cannot be proven" }
  contract_assert (x > 0);	// conditional
  return 0;
}

int i_diagnose_always (int x, int y)
{
  sem_diagnose_always::h (y);	// { dg-warning "contract assertion cannot be proven" }
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

namespace sem_diagnose_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }	// { dg-message "the predicate of the contract assertion" }
}

int d_diagnose_never (int x)
{
  sem_diagnose_never::h (x);	// { dg-warning "contract assertion cannot be proven" }
  contract_assert (x > 0);	// conditional
  return 0;
}

int i_diagnose_never (int x, int y)
{
  sem_diagnose_never::h (y);	// { dg-warning "contract assertion cannot be proven" }
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

// { dg-final { scan-tree-dump {\(line 31\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 39\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 50\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 58\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 69\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 77\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 88\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 96\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 107\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 115\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 126\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 134\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 145\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 153\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 164\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 172\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 183\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 191\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 202\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 210\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 221\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 229\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 240\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 248\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 259\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 267\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 278\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 286\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 296\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 297\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 303\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 305\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 315\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 316\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 322\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 324\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 334\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 335\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 341\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 343\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 353\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 354\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 360\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 362\): conditional} "contract_shadow" } }
