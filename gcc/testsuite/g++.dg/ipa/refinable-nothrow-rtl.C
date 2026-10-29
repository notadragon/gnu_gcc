// A refinable body is not found nothrow after RTL expansion either
// (set_nothrow_function_flags), so the callers compiled after it keep
// their landing pads.
// { dg-do compile }
// { dg-options "-O2 -fno-ipa-pure-const -fno-ipa-modref -fno-ipa-icf -fdump-tree-optimized --param=ipa-refinable-inline=1" }
// The call to EXT keeps the front end and pass_nothrow from finding the
// bodies nothrow; it is folded away later, and with pure-const and
// mod-ref disabled only the RTL pass can find them nothrow.

#define NI [[gnu::noinline]]

static int never_set;
void ext ();
struct S { ~S (); };

// M_RTL is marked (declared inline); U_RTL is the unmarked control.
NI inline void m_rtl () { if (never_set) ext (); }
template <int> NI void u_rtl () { if (never_set) ext (); }
void use_m_rtl () { S s; m_rtl (); }
void use_u_rtl () { S s; u_rtl<0> (); }

// Only USE_M_RTL keeps a landing pad.
// { dg-final { scan-tree-dump-times "__builtin_unwind_resume" 1 "optimized" } }
