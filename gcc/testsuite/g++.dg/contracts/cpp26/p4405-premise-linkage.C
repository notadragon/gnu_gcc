// P4405 premises decided by this TU (T1-GCC): the callee's linkage and
// inlining.  A callee-side enforce precondition of a callee that is static,
// external, inline, a template specialization or weak, and always_inline,
// noinline or left to the inliner.  The shadow may rely on what it inlined
// from a replaceable (inline, comdat, weak) callee only where the real
// function inlined it too; a non-replaceable callee's out-of-line copy is
// this TU's own code.  A call the shadow makes out of line to a replaceable
// callee holding such an evaluation makes it conditional (IPA summaries of
// that callee may rest on it).  GCC never inlines a weak function.
//
// Each consumer is a diagnose evaluation: d_* can be proven only from the
// callee's assertion, i_* from a dominating test, so only what makes the
// whole shadow conditional matters there.  -fdump-tree-contract_shadow
// shows each result ("contract obligation N (line L): sound").
//
// Mirror: clang/test/Contracts/p4405-premise-linkage.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2 -Wno-attributes" }
// { dg-additional-options "-fdump-tree-contract_shadow" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-premise-linkage.json" }

int sink (int);

// static, always
[[gnu::always_inline]] static inline void h_static_always (int x) pre (x > 0) { sink (x); }

int d_static_always (int x)
{
  h_static_always (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_static_always (int x, int y)
{
  h_static_always (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

// static, never
[[gnu::noinline]] static void h_static_never (int x) pre (x > 0) { sink (x); }

int d_static_never (int x)
{
  h_static_never (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_static_never (int x, int y)
{
  h_static_never (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

// static, auto
static void h_static_auto (int x) pre (x > 0) { sink (x); }

int d_static_auto (int x)
{
  h_static_auto (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_static_auto (int x, int y)
{
  h_static_auto (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

// extern, always
[[gnu::always_inline]] void h_extern_always (int x) pre (x > 0) { sink (x); }

int d_extern_always (int x)
{
  h_extern_always (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_extern_always (int x, int y)
{
  h_extern_always (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

// extern, never
[[gnu::noinline]] void h_extern_never (int x) pre (x > 0) { sink (x); }

int d_extern_never (int x)
{
  h_extern_never (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_extern_never (int x, int y)
{
  h_extern_never (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

// extern, auto
void h_extern_auto (int x) pre (x > 0) { sink (x); }

int d_extern_auto (int x)
{
  h_extern_auto (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_extern_auto (int x, int y)
{
  h_extern_auto (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

// inline, always
[[gnu::always_inline]] inline void h_inline_always (int x) pre (x > 0) { sink (x); }

int d_inline_always (int x)
{
  h_inline_always (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_inline_always (int x, int y)
{
  h_inline_always (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

// inline, never
[[gnu::noinline]] inline void h_inline_never (int x) pre (x > 0) { sink (x); }

int d_inline_never (int x)
{
  h_inline_never (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_inline_never (int x, int y)
{
  h_inline_never (y);
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

// inline, auto
inline void h_inline_auto (int x) pre (x > 0) { sink (x); }

int d_inline_auto (int x)
{
  h_inline_auto (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_inline_auto (int x, int y)
{
  h_inline_auto (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

// template, always
template <class T>
[[gnu::always_inline]] void h_template_always (T x) pre (x > 0) { sink (x); }

int d_template_always (int x)
{
  h_template_always<int> (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_template_always (int x, int y)
{
  h_template_always<int> (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

// template, never
template <class T>
[[gnu::noinline]] void h_template_never (T x) pre (x > 0) { sink (x); }

int d_template_never (int x)
{
  h_template_never<int> (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_template_never (int x, int y)
{
  h_template_never<int> (y);
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

// template, auto
template <class T>
void h_template_auto (T x) pre (x > 0) { sink (x); }

int d_template_auto (int x)
{
  h_template_auto<int> (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_template_auto (int x, int y)
{
  h_template_auto<int> (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

// weak, never
[[gnu::weak]] [[gnu::noinline]] void h_weak_never (int x) pre (x > 0) { sink (x); }

int d_weak_never (int x)
{
  h_weak_never (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_weak_never (int x, int y)
{
  h_weak_never (y);
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

// weak, auto
[[gnu::weak]] void h_weak_auto (int x) pre (x > 0) { sink (x); }

int d_weak_auto (int x)
{
  h_weak_auto (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_weak_auto (int x, int y)
{
  h_weak_auto (y);
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

// { dg-final { scan-tree-dump {\(line 30\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 38\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 48\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 56\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 66\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 74\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 84\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 92\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 102\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 110\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 120\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 128\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 138\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 146\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 156\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 164\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 174\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 182\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 193\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 201\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 212\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 220\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 231\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 239\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 249\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 257\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 267\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 275\): conditional} "contract_shadow" } }
