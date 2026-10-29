# GCC-899: `identification_label` is narrower than a range of `string_view`s

**Kind:** defect (library concept and compiler read a subset of the
proposed `group_names` forms)
**Status:** Open -- filed 2026-10-23; not fixed (recorded by instruction)
**Affects:** `std::contracts::labels::identification_label`,
`__group_count`, `__group_max_len` and the combined-label merge
(`__gn_append`) in `libstdc++-v3/include/std/contracts`; the compiler's
`ensure_contract_groups` in `gcc/cp/contracts.cc`
**Workaround:** declare `group_names` as an array of strings or of
`const char *` (or a `std::array` of either)
**Clang:** libc++ (`libcxx/include/contracts` in llvm_llvm-project) has the
same concept and the same sizeof-based count; no CLANG row or mirror test
yet

## Symptom

[`open-bug-identification-label-range.C`](../../gcc/testsuite/g++.dg/contracts/cpp26/open-bug-identification-label-range.C),
measured 2026-10-23 with the installed compiler (`g++ (GCC) 17.0.0
20261019`, whose `<contracts>` is identical to the branch head's) at
`-std=c++26` and `-std=c++29`:

| `group_names` declared as | `identification_label` | groups read |
|---|---|---|
| `const char *const [2]` (control) | true | both |
| `std::array<std::string_view, 2>` | **false** | **none** |
| `std::string_view [2]` | **false** | **none** |
| `std::initializer_list<const char *>` | **false** | **none** |
| `std::span<const char *const>` over 3 names | true | **first 2 only** |

"Groups read" is observed through `-fcontract-group-evaluation-semantic=
safety:ignore` with every label naming `safety` last: a violated
precondition whose groups are missed is enforced during constant
evaluation ("contract predicate is false in constant expression").

Combining the span label with another identification label
(`span_t{} | ptr_array_t{}`) is rejected outright: `__gn_append` copies
`sizeof (const char *)` characters from each "row" and indexes past the
two-element count ("array subscript value '2' is outside the bounds of
array type 'const char [2]'", reported inside `<contracts>`, so the watch
test does not assert it).

## Cause

D3400R5 [support.contract.control.ident]:

    template<class T>
      concept identification_label =
        assertion_control_object<T> &&
        requires (const T t) {
          requires is_const_v<decltype(t.group_names)>;
          { t.group_names } -> ranges::range;
          { *(ranges::begin(t.group_names)) }
              -> convertible_to<string_view>;
        };

The library renders it as `t.group_names[0]` convertible to `const char *`
(its comment says so, to avoid pulling `<ranges>` and `<string_view>` into
the header), and counts names as `sizeof (group_names) / sizeof
(group_names[0])`, which is right only for an array.  The compiler reads
the names the same way: `grok_array_decl` for each index up to that
sizeof quotient, each converted to `const char *`, silently skipping any
element that does not convert.

## Fix

Both sides together.  Library: spell the concept as the wording does, and
replace `__group_count`/`__group_max_len`/`__gn_append` with iteration
over the range (lengths through `string_view`).  Compiler:
`ensure_contract_groups` must iterate the range at translation time --
e.g. evaluate a library helper that yields the names as constant strings
-- rather than indexing by a sizeof count and converting to
`const char *`.  Mirror in libc++ and Clang.
