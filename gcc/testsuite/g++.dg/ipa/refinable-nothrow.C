// A refinable body is not found nothrow, neither by pass_nothrow nor by
// local or IPA pure-const.
// { dg-do compile }
// { dg-options "-O2 -fnon-call-exceptions -fno-ipa-icf -fdump-tree-nothrow -fdump-tree-local-pure-const1 -fdump-ipa-pure-const --param=ipa-refinable-inline=1" }
// -fnon-call-exceptions keeps the front end from setting TREE_NOTHROW
// itself.

#define NI [[gnu::noinline]]

struct S { ~S (); };
int gx;

// M_NT is marked (declared inline); U_NT is the unmarked control.
NI inline void m_nt () { gx = 1; }
template <int> NI void u_nt () { gx = 2; }
void use_m_nt () { S s; m_nt (); }
void use_u_nt () { S s; u_nt<0> (); }

// { dg-final { scan-tree-dump-not "found to be nothrow: m_nt" "nothrow" } }
// { dg-final { scan-tree-dump "found to be nothrow: u_nt" "nothrow" } }
// { dg-final { scan-tree-dump-not "found to be nothrow: m_nt" "local-pure-const1" } }
// { dg-final { scan-tree-dump "found to be nothrow: u_nt" "local-pure-const1" } }
// { dg-final { scan-ipa-dump-not "found to be nothrow: void m_nt" "pure-const" } }
// { dg-final { scan-ipa-dump "found to be nothrow: void u_nt" "pure-const" } }
