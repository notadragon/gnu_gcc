// GCC-24 / PR127173: a noexcept function with a contract segfaults the
// compiler under -fno-enforce-eh-specs.
//
// Extracted from the fix commit's own DejaGnu test,
// gcc/testsuite/g++.dg/contracts/cpp26/pr127173.C (added by gnu_gcc commit
// 5fc7de5d66e), found via `git show 5fc7de5d66e --stat` and read with
// `git show 5fc7de5d66e:gcc/testsuite/g++.dg/contracts/cpp26/pr127173.C`.
// DejaGnu directive lines were stripped; the C++ source, including the
// test's own commentary, was kept as-is. Requires
// -fcontracts -fno-enforce-eh-specs (C++26).

/* PR c++/127173 -- a noexcept function with a contract segfaulted the
   compiler under -fno-enforce-eh-specs.

   maybe_apply_function_contracts predicted the shape of the function body
   from the exception specification: a noexcept function's body "will be
   wrapped in a MUST_NOT_THROW expression", so it took that wrapper's first
   operand.  The wrapper comes from begin_eh_spec_block, which
   use_eh_spec_block gates on flag_enforce_eh_specs -- so under
   -fno-enforce-eh-specs there is no wrapper, and whatever the body did start
   with was read as one.  A checking build asserted; a release build
   segfaulted, which is what the reporter saw.

   The reporter used a MIPS cross-compiler; it reproduces on any target.  */

void f (int x) noexcept pre (x > 1) { }

int g (int x) noexcept post (r : r > 0) { return x; }

int h (int x) noexcept pre (x > 0) post (r : r > 0) { return x; }

/* A noexcept(expr) that evaluates to true takes the same path.  */
void i (int x) noexcept (true) pre (x > 1) { }

/* ... and one that evaluates to false does not, which is the control.  */
void j (int x) noexcept (false) pre (x > 1) { }

/* A noexcept member, and a noexcept function template.  */
struct S {
  void m (int x) noexcept pre (x > 0) { }
  int n (int x) noexcept post (r : r > 0) { return x; }
};

template<typename T> void t (T x) noexcept pre (x > 0) { }
template void t<int> (int);

/* contract_assert inside a noexcept function body.  */
void k (int x) noexcept { contract_assert (x > 0); }
