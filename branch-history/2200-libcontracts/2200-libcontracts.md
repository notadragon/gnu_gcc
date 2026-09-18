---
id: 2200-libcontracts
subject: 'libcontracts: add the contract-violation runtime library'
depends: []
regenerates:
  - Makefile.in
  - configure
fixes: [gcc-21, gcc-37, gcc-38]
---

## Rationale

Add `libcontracts`, a new top-level target library, and wire it in as the
runtime home for the contract-violation ABI.  This is the commit that gives
the branch a place to put contract-violation handling that is not the
compiler and not libstdc++: a small target library, built the way every
other target library in the tree is built, that C, C++, and eventually a
non-GNU frontend can all link against without depending on libstdc++.

The library itself is about 1,200 hand-written lines: `contracts-abi.h`
defines the wire format shared with the compiler (the `__cxa_source_location`
layout, the `CXA_*` enums, the data-block struct); `dispatch.c` and
`accessors.c` implement the default violation handler and the accessors a
violation report exposes; `c_api.c` is the pure-C entry surface
(`__cxa_...`) the compiler's generated code calls; `libcontracts.map`
version-scripts the exported symbols. Alongside it, roughly 15,900 lines are
autotools output -- `libcontracts/configure`, `libcontracts/aclocal.m4`, and
`libcontracts/Makefile.in`, generated from `libcontracts/configure.ac`,
`libcontracts/configure.tgt` and `libcontracts/Makefile.am` -- plus the small
hand-edits to the top-level `Makefile.def`/`Makefile.in`/`configure` that
register libcontracts as a new target module. The generated files are
attributed to this commit because their inputs are, but they were not
themselves regenerated for this mapping: no `autoreconf` pass has been run
over them.

On the compiler side, `gcc/cp/contracts.cc` and `contracts.h` gain the
data-block builders (`build_contract_data_block_constant`,
`build_contract_data_block_ctor`, `init_contract_data_block_types`,
`init_contract_descriptor_tables`, ...), the `__cxa_` entry-point declarators
(`declare_cxa_entry_point`, `declare_handle_contract_violation`,
`declare_one_violation_handler_wrapper`, `declare_terminate_wrapper`), and the
mirror of the ABI constants in `contracts.h` -- restated by hand because the
compiler cannot include a target library's header, with a comment saying so
explicitly. `g++spec.cc` and `decl.cc` teach the driver and `grokfndecl` to
link and declare against the new library. `libstdc++-v3/src/c++26/contract26.cc`
is the C++ side of the same ABI: `contract_violation`'s member functions,
`__handle_contract_violation_default`, and the default-handler invocation
that P3290 hooks into later.

The C++ side is more than that one file.
`libstdc++-v3/include/bits/contracts_abi.h` is the C++ twin of
`libcontracts/contracts-abi.h`: the same wire-format enums and field IDs
restated for C++, plus the templated `__cxa_find_field_ptr<T>` and
`__cxa_find_field_value<T>` accessors through which every
`contract_violation` member function that reads the chain -- `kind()`,
`semantic()`, `detection_mode()`, `comment()`, `message()`, `location()` --
gets its answer.  `contract_violation` itself is reshaped to match: the
seven `_M_version`/`_M_comment`/`_M_src_loc_ptr`-style members collapse
into a single `const __cxxabiv1::__cxa_contract_data_block* _M_chain`, the
inline
accessors become out-of-line declarations, and an explicit chain-taking
constructor is added for the ABI runtime to use.
`libstdc++-v3/src/c++26/contracts_abi.cc` holds the part of the ABI that
cannot be written in C: the `_noexcept` entry points, each a
`try { ... } catch (...) { std::terminate(); }` barrier around a libcontracts
dispatch primitive, and P3100's pure-virtual-call termini.

Housing all of that is what the build-system reorganization here is for.
`libstdc++-v3/src/experimental/contract26.cc`, upstream's original, minimal
`contract_violation` implementation, is deleted outright -- it cannot
survive the reshaping above, since it calls `violation.mode()`, which
becomes `detection_mode()` in this commit.  Its replacement in `src/c++26`,
together with `contracts_abi.cc`, becomes a convenience library of its own,
`libc++26contractsconvenience.la`, compiled with
`-fcontracts -fcontracts-p3290 -fcontracts-p4301`, flags the rest of the
ordinary, non-contracts C++26 runtime must not be compiled with.  The
`Makefile.am`/`Makefile.in` edits in both `src/experimental` and `src/c++26`
are that split, not incidental autotools churn; a reviewer should check the
two per-file custom `$(LTCXXCOMPILE)`/`$(CXXCOMPILE)` rules rather than
assume the generated `Makefile.in` is a mechanical `automake` re-run.  The
`src/Makefile.am` comment this commit adds -- that the contracts runtime is
deliberately kept out of `libstdc++.so` and lives in `libstdc++exp`, with
the pure-C core in `libcontracts` -- describes exactly that arrangement, and
is the reason the reorganization belongs with the library rather than with
`5200-p4301`, whose paper contributes only `contract_violation::report()`.

This commit is also credited with fixing GCC-37 and GCC-38 (both upstream
`std::source_location`-layout bugs: an ICE when the user redeclares the name
as something else, and a linkage change when a contract is seen before
`<source_location>` is included).  It does not fix them by patching the
lookup; it removes the branch's dependence on `std::source_location`'s
layout by giving the ABI its own POD `CXA_FIELD_SOURCE_LOCATION` field
instead.  The actual deletion of the front-end lookup path that used to
depend on that layout is `2600-source-location-machinery`, on top of this
one; both bug-report entries name the pair together.

## Compile gap

The library and its compiler-side callers land together, so nothing in
`libcontracts` itself needs a stub.  Owning the C++ side of the ABI does add
gaps, and they are listed after the emitter residue below.

The contract-check emitter first.  Its lines belong to the commits that own
the machinery they call, which leaves three items in this one.

* The emitter's function header.  A hunk of this commit removes
  `tree build_contract_check (tree contract)` -- the removal shares one
  indivisible hunk with the data-block infrastructure comment that opens the
  new machinery, so it cannot be moved.  The header that replaces it,
  `static tree emit_check_for_semantic (tree contract,
  contract_evaluation_semantic semantic, tree shared_data_addr = NULL_TREE)`,
  is P3595's refactoring and lands with `2400-p3595`.  On its own this commit
  would have to carry the old header plus the
  `contract_evaluation_semantic semantic = get_evaluation_semantic (contract);`
  local, both of which `2400-p3595` then deletes.

* `is_noexcept`.  The `declare_cxa_entry_point` calls this commit adds pass
  it, but the local is P4298's and arrives with `4800-p4298`.  On its own this
  commit would pass `false`.

* Nothing is owed for `shared_data_addr`.  The `gcc/cp/contracts.cc#202` cut
  in `branch-history/splits.md` deliberately runs to four pieces so that this
  commit keeps the unconditional data-block build *and* both closing braces:
  the emitter is brace-balanced here, and `2400-p3595` adds the
  `if (shared_data_addr)` arm on top of it.

Three more come with the C++ side of the ABI, all of them in
`src/c++26/contract26.cc` and its build rules:

* `contract_violation::report()` -- **nothing owed, by construction.**  The
  default handler's opening
  `if (const char* __r = violation.report(); __r && __r[0] != '\0')` stanza
  is not in this commit at all: the
  `libstdc++-v3/src/c++26/contract26.cc#0.1` cut in
  `branch-history/splits.md` carves those three lines out of
  `__handle_contract_violation_default` and gives them to `5200-p4301`,
  which is the commit that adds `report()`.  A reader of this commit sees a
  handler that never mentions reports, which is exactly right for a history
  in which no producer exists yet.

* `contract_violation::message()`.  The same default handler prints it, and
  this commit declares it as part of the member list, but the out-of-line
  definition is `3600-p3099`'s.  On its own this commit would have to carry
  a `return nullptr;` body, which `3600-p3099` replaces with the real
  `CXA_FIELD_MESSAGE` lookup.  `query_control_object()`, the other member
  this commit declares but does not define (`4280-p3400-facet-queryable`
  does), is never called here, so it owes nothing.

* `is_terminating()` -- **one character of scaffolding.**  Its body is a
  three-term disjunction and the third term,
  `|| __s == evaluation_semantic::noexcept_enforce;`, is P4298's.  The
  `libstdc++-v3/src/c++26/contract26.cc#0.12` cut in
  `branch-history/splits.md` gives that line to `4800-p4298` -- but the line
  also carries the statement's `;`, and no line boundary separates the
  terminator from the term.  So this commit must carry a `;` at the end of
  its two-term expression, which `4800-p4298` overwrites when it adds the
  third term.  Behaviourally the two-term version is correct here: a
  semantic that does not exist yet cannot be reported as terminating.

* The default handler's semantic switch names all six semantics, two of
  which -- `noexcept_observe` and `noexcept_enforce` -- are P4298's.  The
  same file's `#0.1` cut hands those two `case`/print/`break` triples to
  `4800-p4298`, so nothing is owed: the `default:` arm this commit keeps
  already prints `unknown(N)` for a semantic it does not name.

* The per-file compile rules.  `src/c++26/Makefile.am` and its generated
  `Makefile.in` build `contract26` with
  `-fcontracts -fcontracts-p3290 -fcontracts-p4301` and `contracts_abi` with
  `-fcontracts -fcontracts-p3290`; the driver does not accept
  `-fcontracts-p3290` until `3300-p3290` adds it to `c.opt`, nor
  `-fcontracts-p4301` until `5200-p4301` does.  On its own this commit would
  name `-fcontracts` alone in all four rules, and the two later commits
  would each add their own flag back.  Splitting the rules instead is not
  possible: a cut moves whole lines, and each flag shares a line with the
  compile command that carries it.

## Contents

- Makefile.def : *
- Makefile.in : *
- configure : *
- configure.ac : *
- gcc/Makefile.in : /ginclude\/contracts\.h/
- gcc/cp/contracts.cc : #201:1, #203, #204:1, #204:3, #205, #206, #207, #208, #209, #210, @build_contract_data_block_constant, @build_contract_data_block_ctor, @build_contract_handler_call, @build_descriptor_table_type, @build_quick_enforce_reaction, @build_record_type_from_arrays, @build_terminate_wrapper, @char, @declare_cxa_entry_point, @declare_handle_contract_violation, @declare_one_violation_handler_wrapper, @declare_terminate_wrapper, @emit_builtin_observable_checkpoint, @get_nth_field, @init_contract_data_block_types, @init_contract_descriptor_tables, @lookup_std_contracts_type, @maybe_emit_violation_handler_wrappers, @remap_retval
- gcc/cp/contracts.h : /THIS IS A MIRROR of libcontracts/, @build_contract_check, @cxa_field_id, @cxa_vendor_id
- gcc/cp/decl.cc : @grokfndecl
- gcc/cp/g++spec.cc : /4 \* need_experimental/, /__contract_invoke_default_handler/, /generate_option \(OPT_l, "contracts", 1, CL_DRIVER,/, /^\tcase OPT_std_c__26:$/
- gcc/ginclude/contracts.h : *
- gcc/ginclude/stdcontracts.h : *
- gcc/testsuite/lib/g++.exp : *
- libcontracts/Makefile.am : *
- libcontracts/Makefile.in : *
- libcontracts/accessors.c : *
- libcontracts/aclocal.m4 : *
- libcontracts/c_api.c : *
- libcontracts/configure : *
- libcontracts/configure.ac : *
- libcontracts/configure.tgt : *
- libcontracts/contracts-abi.h : *
- libcontracts/dispatch.c : *
- libcontracts/libcontracts.map : *
- libstdc++-v3/config/abi/pre/gnu.ver : *
- libstdc++-v3/include/Makefile.am : /contracts_abi\.h/
- libstdc++-v3/include/Makefile.in : /contracts_abi\.h/
- libstdc++-v3/include/bits/contracts_abi.h : *
- libstdc++-v3/include/std/contracts : /contracts_abi\.h/, /unspecified = 0/, /_M_chain/, #12:0, #12:2
- libstdc++-v3/src/Makefile.am : *
- libstdc++-v3/src/Makefile.in : *
- libstdc++-v3/src/c++26/Makefile.am : *
- libstdc++-v3/src/c++26/Makefile.in : *
- libstdc++-v3/src/c++26/contract26.cc : /This file requires C\+\+26 contracts support/, @namespace, @_GLIBCXX_BEGIN_NAMESPACE_VERSION, @_GLIBCXX_VISIBILITY, @_GLIBCXX_END_NAMESPACE_VERSION, @contracts, @__handle_contract_violation_default:0, @__handle_contract_violation_default:2, @__handle_contract_violation_default:4, @assertion_kind, @evaluation_semantic, @detection_mode, @source_location, @is_terminating:0, @is_terminating:2, @__attribute__, @invoke_default_contract_violation_handler, @_Z25handle_contract_violationRKNSt9contracts18contract_violationE, @_Z27__handle_contract_violationRKNSt9contracts18contract_violationE, @_Z41invoke_default_contract_violation_handlerRKNSt9contracts18contract_violationE, @__contract_invoke_default_handler, /^contract_violation::comment\(\) const noexcept$/
- libstdc++-v3/src/c++26/contracts_abi.cc : *
- libstdc++-v3/src/experimental/Makefile.am : *
- libstdc++-v3/src/experimental/Makefile.in : *
- libstdc++-v3/src/experimental/contract26.cc : *
- libstdc++-v3/testsuite/18_support/contracts/is_terminating.cc : *
- libstdc++-v3/testsuite/lib/libstdc++.exp : *
