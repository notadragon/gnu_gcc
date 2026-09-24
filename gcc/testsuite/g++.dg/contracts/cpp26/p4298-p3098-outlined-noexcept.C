// D4298: an outlined postcondition function whose only contract is
// noexcept_enforce is itself noexcept, even though the original function
// is not declared noexcept.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_enforce -fcontract-checks-outlined -fdump-tree-gimple" }

int f(int x) post(r: r > 0) { return x; }
static_assert(noexcept(f(1)) == false);  // f itself is unaffected

// The outlined postcondition-check function's own noexcept-ness is not
// nameable from user code, so it is verified via the gimple dump instead:
// a genuinely noexcept function has its whole body wrapped by the
// compiler in a must-not-throw EH region (begin_eh_spec_block /
// gimplify_must_not_throw_expr), which the gimple dump renders as
// "eh_must_not_throw".  Neither f() nor main() is noexcept, so this
// string can only come from the outlined .post function.
int main() { return f(1); }

// { dg-final { scan-tree-dump "eh_must_not_throw" "gimple" } }
