// P3100: a class copied or assigned from a misaligned `*(P *) (c + o)'
// is reported once by the alignment check
// (ub:basic.align.object.alignment.store): the binding of the trivial copy
// constructor's (or assignment operator's) reference parameter is checked,
// and the read of the object through the bound address is the same
// misaligned access, not checked again (DECISIONS.md E120; GCC-879: both
// were reported).  A reference variable and a read through it later are
// reported twice, the binding and the read, as on Clang.  The forms are
// in p3100-align-class-copy.h.
// With .store configured, at -O2.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.store:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-align-class-copy.h"

// { dg-output "gcopy 1(\n|\r\n|\r)pcopy 1(\n|\r\n|\r)passign 1(\n|\r\n|\r)" }
// { dg-output "pinit 1(\n|\r\n|\r)pbrace 1(\n|\r\n|\r)pvar 1(\n|\r\n|\r)" }
// { dg-output "parg 1(\n|\r\n|\r)passignto 1(\n|\r\n|\r)pref 2(\n|\r\n|\r)" }
// { dg-output "pmem 1(\n|\r\n|\r)aligned 0(\n|\r\n|\r)" }
