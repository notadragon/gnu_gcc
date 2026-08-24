// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=alignment -fsanitize-semantic=alignment:assume" }

// P3100 Task 4.1 (UBSan routing, assume): resolving the alignment check to
// "assume" means the check is OFF -- byte identical to a build without
// -fsanitize=alignment.  Realized per function via no_sanitize (cp-gimplify.cc),
// so NO alignment runtime call is emitted.  (assume is exempt from
// -fcontracts-p4298.)

int sink (int *p) { return *p; }

// The type-mismatch runtime handler must NOT be called: check assumed away.
// { dg-final { scan-assembler-not "__ubsan_handle_type_mismatch" } }
