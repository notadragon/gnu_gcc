# GCC-504: A hidden friend with a precondition in a class template: ICE, or a dependent type left in the predicate

**Status:** Fixed here (commit [c72c0e64899d](https://github.com/notadragon/gnu_gcc/commit/c72c0e64899de20a3287f71fcd761fa5bb771009))
**Resolved by:** `4640-contract-substitution-plumbing`
**Component:** c++
**Keywords (ours -- upstream sets its own):** `ice-on-valid-code`,
`rejects-valid`
**Upstream Link:** None found (searched 2026-09-26: Bugzilla REST, summary
and comment text, on `friend contract template`, `friend precondition`,
`dependent_operator_type`, plus every open bug with `contract` in its
summary; PR127291 is friend redeclaration matching, not this.  Earlier
search 2026-09-25, same result)
**Affects:** measured 2026-09-26 -- 16.1.0, 16.2.0, stock trunk
`17.0.0 20260922` (`f008f03eff8`) and this branch behave identically
(review round #12 fan-out, c-b-6)
**Clang:** correct (compiles both cases and checks the precondition)

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] precondition of a friend defined in a class template is not substituted: ICE in cp_fold, or "could not convert ... dependent_operator_type"` |

Attachments:

| File | Description |
|---|---|
| [`hidden-friend-template-pre.cpp`](hidden-friend-template-pre.cpp) | Two cases selected with `-DCASE=1..2` |

````
A friend function defined in a class template, with a precondition, fails
when the friend is instantiated.  Both cases are valid.

```
#if CASE == 1
template <class T> struct TS {
  friend void tg(TS s) pre(s.m > 0) {}
  T m;
};
int main() { tg(TS<int>{1}); }
#elif CASE == 2
template <class T> struct S {
  friend void fr2(S, T t) pre(t > 0) {}
};
int main() { fr2(S<int>{}, 1); }
#endif
```

```
$ g++ -std=c++26 -fcontracts -fsyntax-only -DCASE=1 \
    hidden-friend-template-pre.cpp
hidden-friend-template-pre.cpp: In instantiation of 'void tg(TS<int>)':
hidden-friend-template-pre.cpp:18:16:   required from here
   18 | int main() { tg(TS<int>{1}); }
      |              ~~^~~~~~~~~~~~
hidden-friend-template-pre.cpp:15:38: internal compiler error: Segmentation fault
   15 |   friend void tg(TS s) pre(s.m > 0) {}
      |                                      ^

$ g++ -std=c++26 -fcontracts -fsyntax-only -DCASE=2 \
    hidden-friend-template-pre.cpp
hidden-friend-template-pre.cpp: In instantiation of 'void fr2(S<int>, int)':
hidden-friend-template-pre.cpp:23:17:   required from here
   23 | int main() { fr2(S<int>{}, 1); }
      |              ~~~^~~~~~~~~~~~~
hidden-friend-template-pre.cpp:21:33: error: could not convert '(((const T)t) > 0)' from ''dependent_operator_type' not supported by dump_type<type error>' to 'bool'
   21 |   friend void fr2(S, T t) pre(t > 0) {}
      |                               ~~^~~
      |                                 |
      |                                 'dependent_operator_type' not supported by dump_type<type error>
hidden-friend-template-pre.cpp:21:33: error: in argument to unary !
```

Expected: both compile, and the precondition is checked when the friend is
called.

A release-checking trunk build with -g shows the CASE=1 crash:

```
0x8a8021 cp_fold
0x8a8c36 cp_fold_r
0x13c586a walk_tree_1(...)
0x8a51e5 cp_fold_function(tree_node*)
0x8e05c7 finish_function(bool)
0xa1f68e instantiate_body
0xa0b367 instantiate_decl(tree_node*, bool, bool)
0xa29819 instantiate_pending_templates(int)
```


DISCOVERY

Found while reviewing an implementation of C++26 contracts against the
working draft.


ANALYSIS

Not traced.  CASE=2 shows the uninstantiated (const T)t > 0 reaching the
conversion to bool in the instantiation, so the friend's contract appears
to be carried over from the class template without substitution; CASE=1
then crashes folding the same kind of tree.  tsubst_friend_function
(gcc/cp/pt.cc), which instantiates the friend from the class template, has
no contract handling at all.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       CASE=1  CASE=2
  compiler-explorer   16.1.0                        ICE     rejected
  compiler-explorer   16.2.0                        ICE     rejected
  compiler-explorer   17.0.0 20260922, f008f03eff8  ICE     rejected
  local build -g      17.0.0 20260922, 7aa4b1c5054  ICE     rejected
````

## Our Fix

`gcc/cp/pt.cc`, `regenerate_decl_from_template`: the early exit for a unique friend (PR69836) now substitutes the friend's contracts first; `tsubst_function_decl` had copied them unsubstituted and nothing else substitutes them for a hidden friend.  Regression tests: `hidden-friend-template-pre.C`, `hidden-friend-template-pre-param.C`.

## Notes (ours, not for the report)

Fix direction: substitute the friend's contracts with the class template's
arguments in `tsubst_friend_function`, as for member functions.

Watch tests: `g++.dg/contracts/cpp26/open-bug-hidden-friend-template-pre.C`
(CASE=1, `dg-ice`) and `open-bug-hidden-friend-template-pre-param.C`
(CASE=2, `dg-xfail-if`), mirrored in the Clang fork by the expected pass
`clang/test/Contracts/OpenBugs/hidden-friend-template-pre.cpp`.
