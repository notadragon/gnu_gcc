// GCC-23 / PR124486, PR125403: [dcl.contract.func]/6 is not applied to a
// deleted function or one defaulted on its first declaration.
//
// Combined from the fix commit's own DejaGnu tests -- one shape per
// original file, found via `git show 25bec7a0e6f --stat` and read with
// `git show 25bec7a0e6f:gcc/testsuite/g++.dg/contracts/cpp26/<name>.C`
// for pr124486-deleted.C, pr124486-ctor.C, pr124486-post.C,
// pr124486-copy.C, and pr124486-dtor.C. DejaGnu directive lines were
// stripped; the original tests keep one case per file because, per the
// fix commit's own log message, combining them into one DejaGnu test made
// DejaGnu intermittently stop matching the `= default' diagnostics even
// though the compiler emits each at exactly the expected line -- "not
// root-caused, and not a property of the fix." That is a DejaGnu-specific
// quirk; it does not apply to a plain standalone reproducer read by eye or
// compiled once, so the cases are combined here into a single file.
// Requires -fcontracts (C++26); every case here is expected to be
// diagnosed on a fixed compiler and is accepted (silently, and wrongly) on
// an unfixed one. See gcc-23-contract-on-deleted-or-defaulted-accepted.C
// for what must NOT be rejected.

// PR c++/124486: a deleted function shall not have a
// function-contract-specifier-seq.
struct Del { void f (int x) pre (x > 0) = delete; };
// expected-error "deleted function .* cannot have a function-contract-specifier"

// PR c++/124486 (nor one defaulted on its first declaration), PR c++/125403's
// ICE shape: a postcondition on an in-class defaulted constructor used to
// reach maybe_apply_function_contracts and ICE there.
struct DefCtorPost { DefCtorPost () post (true) = default; int x = 0; };
// expected-error "defaulted on its first declaration cannot have a function-contract-specifier"

struct DefCtor { DefCtor () pre (true) = default; };
// expected-error "defaulted on its first declaration cannot have a function-contract-specifier"

struct DefCopy { DefCopy () = default; DefCopy (const DefCopy&) pre (true) = default; };
// expected-error "defaulted on its first declaration cannot have a function-contract-specifier"

struct DefDtor { ~DefDtor () pre (true) = default; };
// expected-error "defaulted on its first declaration cannot have a function-contract-specifier"
