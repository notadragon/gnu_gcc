// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=object-size -fsanitize-semantic=object-size:noexcept_enforce -O2" }

// object-size shares the type_mismatch runtime handler with alignment and needs
// -O1+ to instrument.  A routed object-size check resolved to noexcept_enforce
// is integrated along the abort code path -> the _abort handler variant.
static char buf[1] alignas (int);
int sink () { return *reinterpret_cast<int *> (&buf[0]); }

// { dg-final { scan-assembler "__ubsan_handle_type_mismatch_v1_abort" } }
