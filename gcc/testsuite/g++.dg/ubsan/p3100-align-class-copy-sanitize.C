// P3100: a class copied or assigned from a misaligned `*(P *) (c + o)'
// is reported once by the alignment check
// (ub:basic.align.object.alignment.store): the binding of the trivial copy
// constructor's (or assignment operator's) reference parameter is checked,
// and the read of the object through the bound address is the same
// misaligned access, not checked again (DECISIONS.md E120; GCC-879: both
// were reported).  A reference variable and a read through it later are
// reported twice, the binding and the read, as on Clang.  The forms are
// in p3100-align-class-copy.h.
// With -fsanitize=alignment routed to the handler and the P3100 leaf not
// configured, the sanitizer's checks are stock -fsanitize=alignment's,
// which reports the binding and the read both (as without
// -fcontracts-p3100): twice.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-additional-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fsanitize=alignment -fsanitize-semantic=alignment:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "../contracts/cpp26/p3100-align-class-copy.h"

// { dg-output "gcopy 2(\n|\r\n|\r)pcopy 2(\n|\r\n|\r)passign 2(\n|\r\n|\r)" }
// { dg-output "pinit 2(\n|\r\n|\r)pbrace 2(\n|\r\n|\r)pvar 2(\n|\r\n|\r)" }
// { dg-output "parg 2(\n|\r\n|\r)passignto 1(\n|\r\n|\r)pref 2(\n|\r\n|\r)" }
// { dg-output "pmem 1(\n|\r\n|\r)aligned 0(\n|\r\n|\r)" }
