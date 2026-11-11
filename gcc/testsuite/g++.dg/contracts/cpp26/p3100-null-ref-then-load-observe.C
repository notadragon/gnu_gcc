// A null check the front end builds (binding a reference) and one the ubsan
// pass builds (the load through it) on the same pointer, routed to the
// contract-violation handler: sanopt merges them exactly when it does for
// native UBSan.  Under a terminating semantic the routed check does not
// recover, so the dominated check goes and each function keeps one; under
// observe it recovers, and the two are distinct reports at distinct
// locations ("reference binding" and "load"), so both stay -- as native
// -fsanitize-recover=null keeps both (GCC-202).
// { dg-do compile }
// { dg-options "-std=c++26 -O2 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=null -fsanitize-semantic=null:noexcept_observe -fdump-tree-optimized" }

struct S { int x; };
int bind_then_load (int *p) { int &r = *p; return r; }
int ref_member (S *s) { S &r = *s; return r.x; }

// { dg-final { scan-tree-dump-times "__builtin___ubsan_handle_type_mismatch" 4 "optimized" } }
