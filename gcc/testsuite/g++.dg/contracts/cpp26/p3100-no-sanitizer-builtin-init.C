// P3100: -fcontracts-p3100 makes pass_ubsan run for every function so implicit
// contract-assertion checks can be inserted, even with no -fsanitize enabled.
// A function with no instrumentable UB must still compile: the pass's
// initialize_sanitizer_builtins() call creates the sanitizer builtins, and when
// that first happens inside a function body (current_function_decl in scope)
// pushdecl routed the FUNCTION_DECL through set_decl_context_in_fn and ICEd.
// cxx_builtin_function now escapes to the top level before pushing.  This test
// must compile cleanly with no sanitizer and no contracts on the function.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }

void empty () {}

// A function taking a reference, mirroring a typical handler signature, is the
// shape that first exposed the ICE (it is emitted before any function with UB).
struct S;
void takes_ref (const S &) {}

int identity (int x) { return x; }
