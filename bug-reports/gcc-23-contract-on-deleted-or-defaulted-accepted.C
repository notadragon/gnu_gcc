// GCC-23 negative-space control: what [dcl.contract.func]/6 must NOT
// reject. The diagnosed shapes are in
// gcc-23-contract-on-deleted-or-defaulted.C.
//
// Extracted from the fix commit's own DejaGnu test,
// gcc/testsuite/g++.dg/contracts/cpp26/pr124486-accepted.C (added by
// gnu_gcc commit 25bec7a0e6f), found via `git show 25bec7a0e6f --stat` and
// read with
// `git show 25bec7a0e6f:gcc/testsuite/g++.dg/contracts/cpp26/pr124486-accepted.C`.
// DejaGnu directive lines were stripped. The original requires
// -fcontracts -fcontracts-p3097, a branch-specific paper flag not
// available on stock GCC; the virtual-function case below is included
// anyway, disclosed as a known, deliberate limitation of what stock GCC
// (and unpatched GCC generally) would reject -- see the parent .md's
// Bug Report and this file's own comments.

// Defaulted, but NOT on its first declaration: the contract lives on the
// earlier declaration, which is where it belongs.
struct OutOfLine { OutOfLine () pre (true); int x = 0; };
OutOfLine::OutOfLine () = default;

// An ordinary function.
void ordinary (int x) pre (x > 0) { }

// P3097 lifts [dcl.contract.func]/6's virtual-function case, and this
// branch implements P3097, so a contract on a virtual function must keep
// working here -- deliberately NOT diagnosed on this branch. This case
// needs -fcontracts-p3097 (this branch's own flag) to compile without a
// diagnostic; on stock GCC (no P3097 support), a contract on a virtual
// function is still rejected outright as a base-P2900 restriction, which
// is expected and is not part of this bug.
struct Base { virtual ~Base () = default; virtual int vf (int x) pre (x > 0); };
int Base::vf (int x) { return x; }
struct Derived : Base { int vf (int x) override pre (x > 0) { return x; } };

// A deleted or defaulted function with no contract is untouched.
struct PlainDeleted { void g () = delete; };
struct PlainDefaulted { PlainDefaulted () = default; int x = 0; };
