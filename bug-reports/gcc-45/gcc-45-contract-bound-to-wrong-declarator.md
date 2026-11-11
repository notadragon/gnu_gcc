# GCC-45: A function-contract-specifier binds to the preceding parameter list rather than to the declarator

**Status:** Fixed here
**Resolved by:** `1240-contract-specifier-at-declarator-position`,
`1250-contract-on-non-function-declarator`
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `accepts-invalid`,
`rejects-valid`, `wrong-code`
**Upstream Link:** [PR127572](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127572)
-- **FILED 2026-09-23**, UNCONFIRMED. (Searched 2026-09-16 and re-checked
2026-09-18 before filing: the whole `component:c++ contracts` list was
enumerated -- 65 bugs -- and filtered for `typedef`, `alias`, `declarator`,
`non-function`, `pointer`, `parameter of`, `silently`, `ignored`, `accepted`
and `drop`; the only two hits were PR127255 and PR127290, both of which are
ours and neither about declarator placement. Targeted queries were run as
well: `contracts typedef`, `contracts declarator`,
`function-contract-specifier`, `contracts ignored`, `contracts non-function`,
`contract typedef-name`, `contract specifier parameter`,
`keywords:accepts-invalid contract`.)

The same defect in the sibling specifier **is** reported, and those two
reports are the best statement of the root cause that exists upstream:

| PR | filed | status | form |
|---|---|---|---|
| [PR123909](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123909) | 2026-01-31 | UNCONFIRMED | "Requires-clause after function is incorrectly parsed as part of the declarator" -- pointer-to-function, both directions |
| [PR94984](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94984) | 2020-05-07 | NEW | requires-clause after an array declarator |

PR123909 cites `init-declarator`, `function-definition` and
`member-declarator` as the grammar positions that carry the clause, and
notes that Clang, MSVC and EDG are all correct.  That is this bug, one
specifier over.  Tracked here as GCC-47.

The nearest contracts report is
[PR124486](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124486)
("[contracts] Missing restriction on defaulted and deleted functions"),
which is the same family -- a missing restriction on where a contract may be
written -- but a different restriction.

**Affects:** measured 2026-09-18, re-measured 2026-09-23 -- identical on
`16.1.0`, `16.2.0` and trunk `17.0.0 20260922 (experimental)`
(`f008f03eff80`).  `13.4.0`, `14.4.0` and `15.3.0` do not accept the
`pre`/`post` syntax at all.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] a function-contract-specifier binds to the preceding parameter list rather than to the declarator` |

Attachments:

| File | Description |
|---|---|
| [`valid-declarator-rejected.cpp`](valid-declarator-rejected.cpp) | group 1, rejects-valid: well-formed, refused |
| [`contract-silently-dropped.cpp`](contract-silently-dropped.cpp) | group 1, wrong-code: accepted, contract never runs -- must be run |
| [`contract-predicate-wrong-scope.cpp`](contract-predicate-wrong-scope.cpp) | group 1, wrong-code: accepted, contract runs against the return type's parameter -- must be run |
| [`contract-on-non-function-declarator.cpp`](contract-on-non-function-declarator.cpp) | group 2, accepts-invalid: ill-formed, accepted with no diagnostic |

````
A function-contract-specifier-seq follows the complete declarator.  g++
instead allows the specifiers as part of a function type and attaches them to
whatever parameters-and-qualifiers immediately precede them.
In the common case this is fine, but there are cases where invalid
positions are allowed (and the specifiers end up silently ignored) and
cases where the specifiers bind to the wrong parameter list (and assorted
bad things happen).

Concretely: g++ looks for the contract immediately after a parameter list, so
it finds whichever parameter list was written last.  For a function returning
a pointer to function, that is the return type's, not the function's:

  int (*f (int i)) (int) pre (i > 0);
          ^^^^^^^          the function's own parameter list
                   ^^^^^   the one the contract is taken to follow

When those two coincide, which is the ordinary case, the feature works.  The
consequences when they do not fall into two groups, and the difference in
impact between them is large, so I have separated them.  `g` is a
namespace-scope bool.

GROUP 1 -- the declaration is a well-formed function declaration and the
contract belongs to it.  These are correct programs, and the contract on them
is wrongly rejected, silently discarded, or checked against the wrong object:

  int f (int i) pre (i > 0);                    correct
  int (*f (int)) (int) pre (g);                 correct
  auto f (int i) -> char (*) [3] pre (i > 0);   correct
  int (*f (int i)) (int) pre (i > 0);           rejected: 'i' not declared
  char (*f (int i)) [3] pre (i > 0);            rejected: syntax error
  auto f (int) -> int (*) (int) pre (g);        discarded, no diagnostic
  long (*f (int i)) (long i) pre (i > 0);       checks the return type's 'i'

GROUP 2 -- the declarator declares no function, so [dcl.decl.general]/6 says
the contract may not be there at all.  The program is ill-formed and g++
accepts it:

  typedef int F (int) pre (true);               discarded, no diagnostic
  using F = int (int) pre (true);               discarded, no diagnostic
  int (*p) (int) pre (true);                    discarded, no diagnostic
  void h (int bar () pre (true));               discarded, no diagnostic


1.  REJECTS-VALID, group 1 -- valid-declarator-rejected.cpp

All four of these are well-formed and all four fail to compile.

  char (*returns_array     (int i)) [3]   pre (i > 0);  // expected initializer
  char (&returns_array_ref (int i)) [3]   pre (i > 0);  // expected initializer
  int  (*returns_fn        (int i)) (int) pre (i > 0);  // 'i' not declared
  int  (S::*returns_memfn  (int i)) (int) pre (i > 0);  // 'i' not declared

The same four declarators with no contract on them are in the file as
controls, and all four are accepted, so the shapes themselves are spellable
and the contract is what breaks them.

The first two fail because the declarator's outermost part is an array, so
the parser is not looking for a contract there at all.  The second two fail
for a different reason: the contract does reach the function -- the same
spelling with a parameter-free predicate is checked, see reproducer 2 -- but
it is parsed after the function's own parameter scope has been left, so i
does not resolve.  The return type's parameter list is unnamed in both rows;
naming it replaces this error with the silent wrong answer in reproducer 3.

2.  WRONG-CODE, group 1, dropped -- contract-silently-dropped.cpp

This half of group 1 compiles and the contract simply never runs.  All three
declarations below are accepted without a diagnostic; only the first attaches
the contract assertion to the function and evaluates it.

  static bool ok = false;

  int  (*returns_fn_classic   (int))  (int)            pre (ok);
  auto   returns_fn_trailing  (int) -> int (*) (int)   pre (ok);
  auto   returns_ref_trailing (int) -> int (&) (int)   post (r : ok);

Each predicate is false at its call, so three violations should be detected.

  $ g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe \
        -o a.out contract-silently-dropped.cpp && ./a.out
  ... one violation, from returns_fn_classic, then "done"

The two trailing-return spellings are dropped: the parameter list sits
inside the type-id, so the contract binds to an abstract declarator that
never becomes a function.  The classic spelling is the control -- it is the
same function without a trailing return type, and it is checked, which is
what shows the other two should be.  The predicates deliberately name no
parameter, so the defect in the next section cannot show and this file
measures the drop alone.


3.  WRONG-CODE, group 1, wrong scope -- contract-predicate-wrong-scope.cpp

The classic spelling is reported, but it is not correct either -- it is
attached to the right function with its predicate parsed in the wrong scope.
The contract is parsed while the return type's parameter scope is still
open, so a name in the predicate resolves to the return type's parameter
rather than to the function's.

In each declaration the function's own parameter is `int i`, and the return
type long (*) (long) declares its own parameter, also spelled i, of type
long.  Only the int should be in scope.

  long (*which_type (int i)) (long i) pre (same_v<decltype (i), int>);
  long (*which_size (int i)) (long i) pre (sizeof (i) == sizeof (int));

Both report a violation: decltype (i) is long, and sizeof (i) is
sizeof (long).  The name found the return type's parameter declaration.
(same_v is a two-line hand-rolled is_same, so the file needs no
<type_traits>.)

The contract is attached and evaluated, not dropped.  A contradiction, false
for every possible value of i, reports -- which a dropped contract cannot do:

  long (*contradiction (int i)) (long i) pre (i > 0 && i <= 0);

So a predicate that merely reads the value is a live check against an object
that was never created.  At -O0 on x86_64:

  int (*f (int a)) (long i) pre (i > 0);

  _Z1fi:
          movl    %edi, -20(%rbp)     # the real parameter, a
          cmpq    $0, -8(%rbp)        # the predicate's i: never written
          jg      .L4

The orphaned parameter is given its own slot in the enclosing function's
frame and the predicate reads it uninitialised.  That row is not measured in
the reproducer, because the value it reads can differ from run to run; the
three above are deterministic and establish the same thing.

[basic.scope.param]/1.1 gives the function's own parameters a scope reaching
the end of the init-declarator, and the return type's parameter-declaration-
clause is a separate parameter scope that does not extend here at all.


4.  ACCEPTS-INVALID, group 2 -- contract-on-non-function-declarator.cpp

[dcl.decl.general]/6: the function-contract-specifier-seq of an
init-declarator "shall not be present unless the declarator declares a
function", and the note in [dcl.contract.func]/8 says a pointer to function,
a pointer to member function and a function type alias cannot carry one at
all.  Four kinds of declarator take one anyway.  None of them declares a
function, so there is nothing for the contract to belong to and no call is
ever checked.

  typedef int FTypedef (int) pre (true);
  using FAlias = int (int) pre (true);
  void takes_fn (int bar () pre (true));
  void defines_fn (int bar () pre (true)) { (void) bar; }
  int (*gp) (int) pre (true);

  $ g++ -std=c++26 -fcontracts -fsyntax-only \
        contract-on-non-function-declarator.cpp; echo $?
  0

No diagnostic of any kind, for any of them.  In defines_fn the contract sits
on a parameter of a function that is being defined, and is still dropped.

How much of the dropped predicate is looked at depends on the scope, and
inside a class the answer is none of it.  At namespace scope the predicate is
parsed before being discarded, so a meaningless one is still diagnosed:

  typedef int FBadSyntax (int) pre (1 + * / 2);   // expected primary-expr
  typedef int FBadName   (int) pre (nosuchname);  // not declared in scope

Inside a class being defined, cp_parser_function_contract_specifier takes its
deferred branch and stores a token cache, cp_parser_late_contracts is only
ever driven for a FUNCTION_DECL, and the cache is discarded unparsed.  Only
the balance of the parentheses is checked, by the caching scan itself.  Every
one of these is accepted with no diagnostic:

  struct S {
    typedef int FSyntax (int) pre (1 + * / 2);
    typedef int FName   (int) pre (nosuchname);
    typedef int FWords  (int) pre (a b c d);
    typedef int FString (int) pre ("not a bool");
    typedef int FStmt   (int) pre (return 7);
    using   UWords = int (int) pre (a b c d);
    int    (*mpWords) (int) pre (a b c d);
  };

Only an unbalanced predicate such as pre (%%% ][ &&) is refused there, and
that is the paren-matcher failing to find the closing token, not the
predicate being checked.


ANALYSIS

cp_parser_direct_declarator (gcc/cp/parser.cc) parses a
function-contract-specifier-seq in the CPP_OPEN_PAREN arm of its
direct-declarator loop -- after any parameter list -- guarded only by
flag_contracts, and stores it on the cdk_function declarator node via
make_call_declarator.  That arm runs once per parameter list in the
declarator, and it cannot tell which of them, if any, belongs to the
function being declared: at that point it has not seen the decl-specifiers,
so `typedef` is invisible; it does not know whether it is inside a
parameter-declaration-clause; and it builds declarators inside-out, so an
enclosing pointer declarator has not been reached yet.  Parsing there also
means the predicate is parsed with the wrong parameter scope open, which is
the misresolution in reproducer 3.

The grammar puts the seq somewhere the parser does know the answer: after
the complete declarator and its optional requires-clause, in init-declarator,
function-definition, member-declarator and lambda-declarator.  Those are the
same four positions PR123909 names for the requires-clause.

grokdeclarator (gcc/cp/decl.cc) then accumulates the node's specifiers into
a local in its `case cdk_function:` arm, and reads that local only at the
two grokfndecl calls.  Every other exit lets it go out of scope unread --
which is why the drops in reproducers 2 and 4 are silent.

The requires-clause travels in the same declarator slot and fails in the
same positions, in the complementary direction: it does have the "declares
no function" guards, so most of reproducer 4 is caught, but it applies them
to the wrong node, so all of reproducers 1, 2 and 3 are wrongly rejected for it
too.  Its parameter case is additionally missed, because the guard it hangs
on,

  if (!FUNC_OR_METHOD_TYPE_P (type))

is evaluated before the function-to-pointer adjustment that would make it
true.  That adjustment ([dcl.fct]/5) happens further down, in the
decl_context == PARM handling --

  else if (TREE_CODE (type) == FUNCTION_TYPE)
    type = build_pointer_type (type);

-- after the guard that depends on it.  So a function-typed parameter still
holds a bare FUNCTION_TYPE at the guard, and the question "is this a
function type?" answers yes about a declarator that is about to stop being
one.

Two further details.  For a member typedef or a member alias the predicate
is never even parsed: cp_parser_function_contract_specifier takes its
deferred branch inside a class being defined and stores a DEFERRED_PARSE
token cache, and cp_parser_late_contracts is only ever driven for a
FUNCTION_DECL, so the tokens are discarded.  And set_fn_contract_specifiers
(gcc/cp/contracts.cc) never asserts that its key is a FUNCTION_DECL, so
nothing downstream notices that no specifiers were ever recorded.


VERSIONS -- all on x86_64-linux-gnu, and identical on each

  16.1.0
  16.2.0
  17.0.0 20260922 (experimental), f008f03eff80

  reproducer 1  4 diagnostics on well-formed code    should be 0
  reproducer 2  1 violation of the 3 owed            should be 3
  reproducer 3  which_type, which_size,              should be contradiction
                contradiction and control report     and control only
  reproducer 4  0 diagnostics on ill-formed code     should be nonzero

13.4.0, 14.4.0 and 15.3.0 do not accept the pre/post syntax at all.
````

## Reproducer

Four files in this directory, one per symptom.  The two compile-only ones
include nothing at all and so are their own preprocessed source; the two that
must be run include only `<cstdio>`, to print.

| File | Mode | Stock | Wanted |
|---|---|---|---|
| [`valid-declarator-rejected.cpp`](valid-declarator-rejected.cpp) | `-fsyntax-only` | 4 errors | clean |
| [`contract-silently-dropped.cpp`](contract-silently-dropped.cpp) | build and run | 1 violation | 3 violations |
| [`contract-predicate-wrong-scope.cpp`](contract-predicate-wrong-scope.cpp) | build and run | 4 violations | 2 violations |
| [`contract-on-non-function-declarator.cpp`](contract-on-non-function-declarator.cpp) | `-fsyntax-only` | clean | rejected |

Both wrong-code reproducers **must be run.**  Every declaration in them
compiles clean on stock, so a `-fsyntax-only` measurement reads `clean`
before and after any fix and could never show either the bug or its repair.
That is how the trailing-return drop survived unnoticed in our own test
suite: a compile-only test cannot tell an attached contract from a discarded
one.

They are split because the two failures need different evidence.  The drop is
shown by a violation that does not arrive; the misresolution is shown by one
that arrives and is wrong.  Measuring them in one file meant reading a count
that moved for two unrelated reasons, which is what made the original
reproducer hard to follow.

`contract-predicate-wrong-scope.cpp` is deterministic on purpose.  A
predicate that merely reads the misresolved parameter, `pre (i > 0)`, reads
uninitialised stack and can report either way from run to run; asking
`decltype (i)` and `sizeof (i)` gives the same answer every time, and the
contradiction row proves the contract is evaluated rather than dropped
without depending on any value at all.

## Our Fix

Two commits, in the order the defect has to be taken apart:
`1240-contract-specifier-at-declarator-position` moves the parse to where
the grammar puts the seq, which is what reproducers 1, 2 and 3 need, and
`1250-contract-on-non-function-declarator` diagnoses what is left over,
which is reproducer 4.

**`1240` parses the seq after the complete declarator**, at the four
positions that carry it -- `init-declarator`, `function-definition`,
`member-declarator` and `lambda-declarator` -- and attaches it to the
`cdk_function` nearest the declarator-id, which is the node `grokdeclarator`
visits last and so the one that declares the function.  A position with no
declarator to attach to diagnoses by name instead of dropping.

**Moving the parse forces the predicate to be deferred, everywhere.**
Declarators are built inside-out and a function's own parameter list need
not be the last one, so in `int (*f (int i)) (int) pre (i > 0)` the scope
the predicate needs was destroyed two steps before the contract is seen:
no parse position has the right parameters live.  The predicate is
therefore token-cached -- the mechanism in-class members already used --
and replayed from `start_function_contracts` once the `FUNCTION_DECL`
exists and the parameters can be put back.  A `lambda-declarator` is the
exception, since its parameter list is the declarator, so its scope is
still open and it parses in place.

Deferring is also what fixes the misresolution: the predicate is no longer
parsed while the return type's parameter scope is open, so a name in it can
only find the function's own parameters.

**`1250` adds one check in `grokdeclarator`**, placed directly after the
declarator walk completes, where `type`, `typedef_p` and `decl_context` are
final and every exit that would drop the specifiers is still downstream, so
all of reproducer 4 is caught in one place rather than four.  It diagnoses
and then clears the specifiers, so nothing further along sees a contract
that can never be applied.

The classification is asked once.  `classify_non_function_declarator`
answers "what does this declarator declare, if not a function?" from
`typedef_p`, `decl_context` and the type, and both specifiers read its
answer -- which is what keeps the two from drifting apart as either rule is
extended.  They keep their own wording deliberately: the requires-clause's
messages are upstream's and `1250` does not re-spell them.

The diagnostic names the declarator kind -- typedef, type-id, parameter,
bit-field, non-function type -- rather than saying "not a function", because
the shapes fail for visibly different reasons and a reader needs to know
which one they hit.

`FIELD` is deliberately not rejected wholesale, and that is the part to
check first in any re-spin: a member function declaration arrives with
`decl_context == FIELD`, the same context as the pointer-to-function data
member, so the discriminator has to be the type and the storage class rather
than the context alone.  A rule written one line too broadly silently
rejects every member function that carries a contract.

**The order of the two commits is not cosmetic.**  Diagnosing from
`grokdeclarator` can only ever answer "this declarator declares no
function"; it cannot put a dropped contract back on the function the grammar
assigns it to, and it cannot bring the function's parameters into scope for
a predicate that was parsed too early.  While `1250` stood alone it also
refused `int (*f (int)) (int) pre (ok)` -- a well-formed shape that stock
gets right -- because a guard reached from the old parse position cannot
tell a contract that arrived through the return type's parameter list from
one that arrived on a declarator that declares nothing.  With `1240` in
front of it the question is asked from a position where the answer is
unambiguous, and that regression is gone.  The plan both commits follow is
`src/pubs/impl/p3850impl/contract-declarator-position-gcc-plan.md` in the
papers repository.

The parameter shape needs one thing more than a place to ask the question.
The guard the requires-clause hangs on, `!FUNC_OR_METHOD_TYPE_P (type)`, is
evaluated before the function-to-pointer adjustment of [dcl.fct]/5, so a
function-typed parameter still holds a bare `FUNCTION_TYPE` there and the
guard answers "function type" about a declarator that is about to stop
being one.  A contract check written beside it would inherit that hole;
asking `decl_context == PARM` directly, rather than asking the type, is
what closes it for both specifiers.

Measured 2026-09-23 against the branch install `17.0.0 20260922`:
reproducer 4 gives 24 diagnostics, reproducer 1 is clean, reproducer 2
reports all three violations it owes, and reproducer 3 reports only
`contradiction` and `control` -- that last one confirming `decltype (i)` is
`int` once the predicate is deferred.

The tests are
`gcc/testsuite/g++.dg/contracts/cpp26/contract-on-non-function-declarator.C`
for reproducer 4, `contract-declarator-positions.C` for the declarators that
merely have a function type, and `contract-declarator-positions-run.C`,
which is `dg-do run` and covers reproducer 2 -- the shapes it holds compiled
clean and unchecked for the whole life of this branch, and a `dg-do
compile` version of it would have passed throughout.

**Reproducer 3 has no testsuite row yet**, and that is a gap rather than a
decision.  Every return type's parameter list in
`contract-declarator-positions-run.C` is unnamed, so none of its rows can
tell which declaration the predicate's name resolved to; the branch gets the
answer right, and nothing in the suite would notice if it stopped.  A
`decltype` row belongs there, and its mirror in the Clang tests below.

The Clang mirrors are
`clang/test/Contracts/contract-on-function-typedef.cpp`,
`contract-declarator-positions.cpp` and `contract-in-declarator-group.cpp`;
the first covers fewer rows than ours because Clang already rejected the
parameter and the pointer shapes.

## Notes

The defect was found by comparing this test suite against an independent
Clang contracts implementation, which is correct on all fourteen shapes
measured; nothing in our own suite covered a contract on any of these
declarators.

The misresolution in reproducer 3 was found later, on 2026-09-23, while
re-reviewing this writeup: the original reproducer kept its predicates
parameter-free so that all three shapes would compile on stock, and that is
exactly what hid it.  A `decltype` row now pins which declaration the name
binds to.

GCC-46, which tracked the function-typed parameter shape on its own, was
folded into this row: it is the same parse position, and measuring it
separately said nothing the accepts-invalid reproducer does not.

The requires-clause half is [GCC-47](../gcc-47/gcc-47-requires-clause-on-parameter.md),
and is fixed here by the same commit.  It is **not** filed separately
upstream: PR123909 and PR94984 already cover it, and between them they state
the root cause more precisely than a new report would.  The one
requires-clause datapoint neither PR records is the trailing-return spelling
-- searches for `requires-clause on type-id` and `requires-clause on return
type` both return zero against a control query that does find PR123909 --
and that belongs as a comment on PR123909 rather than as a new bug.
