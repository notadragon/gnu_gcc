---
id: 6110-p3100-check-null-and-alignment
subject: 'c++: contracts: P3100: implicit null-dereference and misaligned-access checks'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
regenerates: []
fixes: []
---

## Rationale

Routed-check ids 2 and 7 -- `-fsanitize=alignment` and `-fsanitize=null` --
together with the two implicit contract assertions that reuse their
machinery: `ub:expr.unary.dereference.nullptr` and
`ub:basic.align.object.alignment`.  Each reports a leaf of its group: a
null pointer is `.nullptr.access` at a load or store and
`.nullptr.indirection` where nothing is accessed (a reference binding, a
member call, the address of a member), and misalignment is `.store`, the
only one of the alignment group's leaves an access-site check can see.

These two checks are one commit because they are one indivisible change.
Both are carried by the same internal function, `IFN_UBSAN_NULL`, whose
operand list this branch widens from three operands to nine -- a null
reaction plus handler entry and data block, and the same three again for
alignment.  `instrument_mem_ref` resolves both reactions at `pass_ubsan`
(pre-inline, where `cfun->decl` is still the function whose namespace the
contract configuration matches) and writes both onto the one call;
`ubsan_expand_null_ifn` reads both back and, when either applies, splits the
alignment condition's true edge into its own then-block so a null contract
and an alignment contract at the same access each fire with their own
semantic.  A single `build_reaction` lambda serves both edges.  Nearly every
hunk in `ubsan_expand_null_ifn` and `instrument_mem_ref` names both
reactions; there is no cut that puts null in one commit and alignment in
another.

`c-ubsan.cc`'s `ubsan_maybe_instrument_reference_or_call` is the front-end
entry to the same IFN, for binding a reference and for the implied `this`
dereference of a member call, and carries the same two reactions.  It also
checks the object pointer of `p->*pm`, where whether the member is read or
written is not yet known: that check is built as `.indirection` whenever
either leaf applies, and `cp_ubsan_retarget_ptrmem_object_checks`, run on
the finished body in `cp_genericize`, retargets it to `.access` where an
indirection outside an address computation reads or writes through it (a
read, store or increment of the member, or of an element or member of it),
so `p->*pm` is reported as `p->m` is.

One null dereference is reported once: the check of the outermost object
pointer suffices.  A reference binding or member call on a member
subobject's address (`p->s.m`, `p->s.f ()`) gets no null reaction of its
own, since the ubsan pass checks `p` where the address is computed
(`ubsan_p3100_checked_subobject_p`).  The object pointer of `p->*pm` is
saved in a temporary of its own, and `pass_ubsan` notes those checked
before it instruments anything (`ubsan_note_ptrmem_object_checks`), so an
address formed from one within the expression -- the member's address
`p + pm`, a member or element of it, through a base conversion or the
null-member-pointer guard -- is not checked again, by the ubsan pass or by
a front-end reference-binding or member-call check
(`ubsan_ptrmem_derived_p`).  For `(p->s).*pm` the pointer checked is `p`
itself (`ubsan_p3100_instrument_ptrmem_subobject`).  A member of a base
class is checked on the object pointer before its conversion to the base,
so a base at a nonzero offset and a virtual base (whose offset is loaded
through the vptr, an access) are reported, by the implicit check and by
`-fsanitize=null` alike; `g++.dg/ubsan/null-base-member.C` pins the latter.
An indirection whose result is not accessed and that leaves nothing in
GENERIC for those checks to see is checked where the front end drops it.
`p3100_indirection_glvalue_p` recognizes the glvalue: `*p`, a cast of `*p`
to a reference (a reference conversion of a pointer that is neither an
object's address nor a base subobject's), or a conditional or comma
expression yielding one.  `&` and `__builtin_addressof` of one fold to the
pointer (`cp_ubsan_maybe_check_address_of`); a call through one for a
pointer to function decays to it (`cp_build_function_call_vec`); a
discarded one (`convert_to_void`), the object expression of a static
member, an enumerator or a static member function (`sp->sf ()`, `sp->k`,
`sp->E`), and the object of a pseudo-destructor or of a trivial
destructor called through a pointer (`build_trivial_dtor_call`) are checked
by `cp_ubsan_maybe_check_unaccessed_glvalue`, which also checks the
pointer of a discarded conversion of `*p` to a base class before it is
converted -- under `.access` for a virtual base, whose offset is read
through the pointer.  `cp_fold` does not fold `std::addressof (*p)` to a
conversion while the leaf applies, so the reference binding stays checked.
All of them go through `cp_ubsan_maybe_check_unaccessed_indirection`, as
`.indirection` (`ubsan_build_p3100_indirection_check`; nothing for
`-fsanitize=null`, which checks none of them), and not for `this`.  These
checks are built at parse time, so they are in constexpr bodies: constant
evaluation ignores a null check as before, except that a speculative one
(folding a call) gives up on a P3100 null check whose pointer is null, so
the check is not folded away; likewise on an unaccessed indirection
through a null pointer while the leaf applies (`cxx_eval_indirect_ref`),
whose check the ubsan pass may build only later (`&pp[0][1]`).
`cp_genericize_r` opens the walk that reaches it when `-fcontracts-p3100` is
on but no sanitizer is.  `lto-streamer-in.cc` is the reason the reactions
are operands rather than a re-resolution: a `.UBSAN_NULL` carrying a
non-`IMPLICIT_UB_NONE` reaction must survive stream-in even with the
sanitizers off, and `-fcontracts-p3100` is a front-end flag that is not in
the LTO options section, so the flag is re-armed for the surviving call.

A reference bound to `*p` and read or written through within the same
expression is one dereference too: the reference parameter of an elided
trivial copy constructor or assignment (`S s2 = *sp`, `s = *sp`, a return,
a class passed by value -- the loop checks such an argument as a load),
`static_cast<int &> (*p)` read or stored to.  The pointer such a reference
is bound to is held in a temporary named `UBSAN_P3100_BOUND_NAME`;
`pass_ubsan` notes those checks (`ubsan_note_binding_checks`), and a load
or store through an address formed from one within the expression
retargets the binding's check to `.access` instead of being checked again
(`ubsan_binding_check_for`, `ubsan_retarget_binding_check`), while the
address alone leaves it `.indirection`.  The check of `&*p` is not such a
check: `(&*p)->m` indirects twice.  A glvalue that is already a
reference -- a reference variable, parameter or member, a call returning a
reference -- indirects through nothing: a reference bound to it, its
conversion to a base, a member function called on it, the object of `.*`
(`ubsan_p3100_reference_glvalue_p`), the address of its member or base
subobject (`instrument_mem_ref`) and a read or write through it
(`ubsan_p3100_reference_pointer_p`: a reference variable or parameter, a
reference read from memory, the result of a call returning one, through
copies, conversions, offsets and member addresses but not through a pointer
variable) get no null reaction, and neither does a pointer converted from
one (`cp_ubsan_maybe_check_unaccessed_indirection`).  With `.access` alone
the binding is still built, with no reaction, so that an access through it
within the expression retargets it; the retargeted check of an address at
a constant offset from a pointer, `S s2 = sp[1]`, checks the pointer, as
the gimplifier's folded access `MEM[sp + 8B]` would be checked without the
binding (unless the offset changes the alignment the check also tests).
The temporary is made under the `.store` alignment check too, and an
access through it whose alignment the binding's check already tests, with
the same reaction, builds no alignment edge of its own
(`ubsan_binding_checks_align_p`): a class copied from a misaligned
`*(P *) (c + o)` is reported once.
While the null check applies, `cp_fold` folds `std::move`,
`std::forward`, `std::forward_like` and `std::as_const` at -O0 as well
(`cp_ubsan_fold_cast_call_p`), so the binding of `*p` to the parameter and
the read through the result are seen as the one dereference they are.  A
trivial copy or move of an empty class reads nothing, and the elided copy
drops its reference binding with its check, so `build_over_call` checks an
indirection it copies from (`cp_ubsan_maybe_check_empty_copy_source`, also
through those calls, and the object a member or array member element it
copies from belongs to) under `.indirection`, or `.access` for a
conversion to a virtual base, also one of an object whose complete type is
known and whose offset is therefore not read
(`p3100_virtual_base_conversion_p`).  A reference bound to, a member
function called on, or the address of a virtual base subobject (or of a
subobject of one) of `*p` -- at a constant offset in an object of known
type, `E &r = p->ve`, `p->vb.f ()`, `&p->vb.b`, or read through a vtable,
`E &r = p->va[1]`, `E &r = p[1]` -- is a conversion to a virtual base, the
`.access` leaf whether or not the offset is read:
`cp_ubsan_maybe_check_vbase_conversion` checks `p`, saved, with the kind the
object pointer of `p->*pm` gets, and forms the address from the saved
pointer, so the ubsan pass checks neither the member's address nor the
vtable pointer's read again (`ubsan_ptrmem_derived_p`) and the binding or
call itself is not checked (`ubsan_p3100_checked_subobject_p`).  An empty base class has no field, so the
address of one at a nonzero offset is `p + offset`, never null: its
reference binding, member call, address and unaccessed uses check `p`
instead (`ubsan_p3100_base_offset_object`,
`ubsan_p3100_check_base_offset_object`).  A member of `*p` evaluated and
not accessed -- discarded (`convert_to_void`), the object expression of a
static member -- is checked on `p` like `*p` itself
(`p3100_subobject_indirection`, `p3100_object_pointer`), a subscript's side
effects kept; the clobber of a trivial destructor of a member is a store
through the member's address, which the ubsan pass checks.  A trivial destructor of a virtual base called through a pointer is checked
on that pointer under `.access`, and its clobber, which would read the
base's offset again, is dropped (`cp_ubsan_virtual_base_glvalue_p`).
`-fsanitize=null` is unchanged: it checks a trivial assignment called
through a pointer, `sp->operator= (s)`, as a store
(`g++.dg/ubsan/null-trivial-assign-call.C`).

The element at a constant subscript of a pointer, `p[c]`, is `*(p + c)`:
its null check is made on `p` itself in every form, as for `p[0]`, unless
the offset would change an alignment the check tests.  `cp_build_array_ref`
marks the element's address `p + c` of a subscript
(`P3100_SUBSCRIPT_ADDRESS_P`, kept by `cp_fold`) where a null leaf
applies; `cp_gimplify_expr` builds `*(p + c)` as `MEM[p + c]`, whose base
the ubsan pass checks for a load, a store and the address of a subobject;
and the reference binding, member call, `p->*pm` object and unaccessed
indirection checks check `p` for the address itself, forming it from the
checked pointer (`ubsan_p3100_constant_subscript`,
`ubsan_p3100_rebuild_subscript`).  A non-constant subscript is checked on
the element's address.  The trivial copy or assignment of an empty class
that `build_over_call` elides also drops an element of an array of known
bound it copies from or stores to: it keeps the element's address, `(void)
&ea[i]`, as a run-time-only guard, so that the array bounds walk checks
it as an address -- one past the end
`ub:expr.unary.dereference.invalid.known`, further
`ub:expr.add.out.of.bounds.known.subscript`
(`cp_ubsan_maybe_check_empty_copy_bounds`), while constant evaluation
still evaluates the copy's source; it travels with the empty copy's null
check it is built beside.
An empty non-virtual base of such an element, copied from or stored to,
is the same copy of an empty class and is checked the same way
(`p3100_empty_base_of_p`).

An indirection through a pointer plus or minus an integral constant
expression, `*(p + c)`, `(p + c)->m`, `(p + c)->*pm`, also `c + p` and
`p - c`, is the subscript spelled out and is checked on `p` the same way:
`cp_build_binary_op` marks the sum it builds for a pointer plus or minus
an integral constant expression (`P3100_ARITH_ADDRESS_P`), and an
indirection through such a sum marks it as an element address.  A
constant that is not a literal, a constant variable `k`, leaves the offset
`k * sizeof (T)` unfolded when the sum is built, so the offset of a marked
sum, of a subscript too, is folded to an integer constant then
(`p3100_fold_element_offset`): the checks built with the expression test
for a constant offset, and `cp_fold`'s non-ODR-use pass would otherwise
rebuild the sum without its mark.  A chain with constants, `p + 1 + 2`
or `(p + 1)[1]`, which `fold` combines into one sum on `p`, is marked too
(its pointer operand a marked sum on the same `p`); a non-constant offset
is not marked.

With `ub:expr.add.out.of.bounds.pointer.arithmetic` configured as well, the
sum is guarded against wrapping around the address space, `force,
(wrapped ? <reaction / p> : p + c)`, with `p` saved; the marks go on the
sum inside the guard, which `ubsan_p3100_unguarded_sum` finds (also when a
conversion of the guard was folded through its comma).  The checks that
check `p` for the address itself do so before the guard and keep the guard
as the address (`ubsan_p3100_guarded_subscript_p`).  An element accessed,
or with the address of a subobject taken, gets the check of `p` in the
arm where nothing wraps, under the leaf the ubsan pass would give it
(`cp_ubsan_check_guarded_subscript`, from the walk that retargets the
`p->*pm` object checks, which skips a clobber's object); in the other arm
the address is `p`, which the ubsan pass checks after the wrap is
reported.  In a chain, `p + 1 + 2`, the outer sum is formed on the inner
one's guard, which `fold` cannot combine with it; the checks then check
the innermost pointer, followed through the guards by
`ubsan_p3100_subscript_pointer`.  A conversion to a reference folded into
the guard's arms binds the reference in each, and a read through the join
is the access of every binding (`ubsan_binding_checks_for`).

A member function called through a pointer to member function has its
object pointer checked before the pointer to member function's `this`
adjustment (`cp_ubsan_maybe_instrument_ptrmemfunc_call`), so that a null
object pointer is reported whatever the adjustment, and an element at a
constant subscript is checked on its pointer there too.  The vtable lookup
of a virtual pointer to member function reads through the adjusted
pointer first: reading the vtable pointer is `.access`, as for a virtual
call, so the lookup checks the object pointer (the element's pointer for
a constant subscript) under that leaf and reads through an address formed
from it, which the ubsan pass then does not check again
(`p3100_check_pmf_vtable_read`).  A `dynamic_cast` to a reference type
reads the vtable pointer too: the pointer its operand is reached through
is checked under `.access` (`cp_ubsan_maybe_check_dynamic_type_read`, from
`build_dynamic_cast_1`, sharing `p3100_check_object_pointer_access` with
the conversion to a virtual base), so the reference binding's own
`.indirection` check is not made.  A virtual function called on a
subobject of `*p` -- a base at a nonzero offset, an element of a member
array -- has the subobject's address computed before the vtable pointer
is read; `cp_ubsan_maybe_check_virtual_call_object` (from
`cp_genericize_r`) checks `p` under `.access` first and forms the
addresses from the saved `p`.  The leaf of a call of a virtual function by
its unqualified name follows the source, not whether the vtable pointer is
read: one the front end resolves statically (a `final` function or class,
a member subobject or an array's first element, whose dynamic type is
known) is marked `CALL_EXPR_P3100_VIRTUAL_P` by `build_over_call`, and its
object pointer is checked under `.access` too -- `p` of a subobject by the
same function, a pointer itself by the member call's own check made under
that leaf (`cp_ubsan_maybe_check_devirtualized_call`).  A call through a pointer to member function
on a member subobject, `(p->m.*pmf) ()`, `((p->*pm).*pmf) ()`, checks `p`
once, after the pointer to member function is evaluated, under the leaf
its virtual bit selects: `.access` before the vtable lookup,
`.indirection` where the function pointer is its own
(`p3100_check_pmf_member_object`); on a member of a virtual base, whose
offset is read through `p` where the object pointer is evaluated, it checks
`p` there, once, under `.access`, whatever the pointer.  These checks are the kind the object
pointer of `OP->*PM` gets (`p3100_object_pointer_check`), so the ubsan
pass checks no address formed from `p` again -- for `-fsanitize=null`
neither, which every such check makes too, so with the sanitizer routed
as well a null dereference is reported once (`instrument_mem_ref`; the
check of `p->*pm`'s object pointer is dropped where a pmf call on its
member checks `p` itself).

No null check is made through `this` -- of the member function being
instrumented, or captured by a lambda (`ubsan_p3100_this_pointer_p`): the
null dereference is reported where the member function is called, so the
count does not depend on the optimization level.  Neither a read or write
through it nor a member's address formed from it is checked, and every
null check the front end built of a pointer formed from it -- a reference
bound to `*this` or to a member (the reference parameter of a copy of
`*this` too), a member function called on it, an unaccessed indirection,
the object pointer of `this->*pm` -- gets no reaction
(`ubsan_p3100_disable_this_check`, `ubsan_disable_binding_check`).  The
object pointer of `this->*pm` or `(*this).*pm`, retargeted to `.access`
for any other object pointer where the member is read or written, gets no
reaction there either (`cp_ubsan_this_pointer_p`).

With `-fsanitize=vptr` too, the vptr check of a conversion to a virtual
base is made before the base's offset is read, and the offset read
through the converted pointer itself (`build_base_path`, `vcheck`), so the
read of `p[1]`'s virtual base offset is checked on `p` as without the
vptr check.

Under `-fcontracts-p3100` one null dereference is reported once with
`-fsanitize=null` too where that needs no state carried between passes
(DECISIONS.md E120): the front end makes no sanitizer check of a member
call's or reference binding's object pointer that is the address of a
subobject whose object pointer the ubsan pass checks
(`ubsan_p3100_checked_subobject_p`, no alignment edge), and the ubsan pass
none of a read through a reference just bound whose binding is checked
(`binding_covered` in `instrument_mem_ref`).  Nor does the ubsan pass keep
the sanitizer's check, with no alignment edge, of a binding or member call
whose pointer is formed from an object pointer the front end checked
(`ubsan_note_ptrmem_object_checks`) -- the operand of a `dynamic_cast` to
a reference, whose object pointer is checked under `.access`.

A reference binding the front end built with no location of its own (an
implicit conversion, a reference constant) is checked at the location of
the innermost expression around it that has one (`cp_genericize_data::loc`),
so that the contract violation names the binding's file and line and
per-site configuration matches it.  And `fold_unary_loc` keeps a
conversion of a sum to a pointer or reference type that needs more
alignment in GENERIC under `-fcontracts-p3100`, as for
`-fsanitize=alignment`: folded, `int &r = *(int *) (buf + o)` would check
the alignment of `buf` alone.

A member call returning a class with a non-trivial copy constructor or
destructor by value is an `AGGR_INIT_EXPR` until gimplification, which
`cp_genericize_r` does not instrument as a call; its object pointer gets
the P3100 check (`cp_ubsan_maybe_check_aggr_init_call`, DECISIONS.md
E113 (b)) -- `.access` for an unqualified virtual call resolved
statically, the flag carried by `build_aggr_init_expr` -- but no
sanitizer check, stock `-fsanitize=null`'s miss staying stock.

The `.def` row for `RUC_ALIGNMENT` is *not* here: it is one net-diff hunk
with the rows for object-size, nonnull-attribute and returns-nonnull-
attribute, so it stays in `6000-p3100-core` with the rest of the table's
indivisible text.  `RUC_NULL` is its own row and does travel here.

## Compile gap

Forward references that would need stubbing if this commit were built alone:
none.  The `UBSAN_NULL_*` operand enumeration and `enum
implicit_ub_reaction` are in `gcc/ubsan.h`, the `IFN_UBSAN_NULL` arity in
`gcc/internal-fn.def`, the `resolve_implicit_ub_semantic` and
`build_implicit_ub_handler` langhooks and their C++ implementations, and the
`pass_ubsan` gate that runs the pass under `-fcontracts-p3100`, are all in
`6000-p3100-core` -- as is `sanopt.cc`, which must already know that two
`IFN_UBSAN_NULL` calls may only be merged when their reactions match.

The semantic trap is the pre-inline resolution.  The reaction is resolved
once, in `pass_ubsan`, and carried on the IFN.  `ubsan_expand_null_ifn` runs
at `sanopt`, post-inline and -- under LTO -- at LTRANS where no C++ langhook
exists; re-resolving there would match the *caller's* namespace against the
contract configuration and silently drop or change the check.  Nothing
diagnoses that; the only symptom is a check that does not fire, which is why
`p3100-nullderef-two-ns.C` and `p3100-nullderef-lto.C` exist.

## Contents

- gcc/c-family/c-ubsan.cc : /#include "langhooks\.h"/, @ubsan_build_p3100_indirection_check, @ubsan_maybe_instrument_reference_or_call, @ubsan_p3100_checked_subobject_p, @ubsan_p3100_instrument_ptrmem_subobject, /^static tree ubsan_maybe_instrument_reference_or_call \(location_t, tree, tree,$/, @ubsan_p3100_reference_glvalue_p, @ubsan_p3100_base_offset_object, @ubsan_p3100_check_base_offset_object, @ubsan_p3100_constant_subscript, @ubsan_p3100_rebuild_subscript, @ubsan_p3100_unguarded_sum, @ubsan_p3100_subscript_pointer, @ubsan_p3100_vptr_checked_downcast, @ubsan_p3100_guarded_subscript_p, /ubsan_maybe_instrument_member_call_object/, /ubsan_maybe_instrument_reference \(tree \*stmt_p|EXPR_HAS_LOCATION \(stmt\) \|\| !flag_contracts_p3100/
- gcc/c-family/c-ubsan.h : /ubsan_build_p3100_indirection_check|ubsan_p3100_base_offset_object|ubsan_p3100_check_base_offset_object|P3100_SUBSCRIPT_ADDRESS_P|P3100_ARITH_ADDRESS_P|ubsan_p3100_constant_subscript|ubsan_p3100_rebuild_subscript|ubsan_maybe_instrument_member_call_object|ubsan_maybe_instrument_reference \(tree \*,/
- gcc/contracts-routed-checks.def : /RUC_NULL/
- gcc/cp/cp-gimplify.cc : /SANITIZE_ALIGNMENT/, /cp_ubsan_maybe_check_vbase_conversion/, /cp_ubsan_retarget_ptrmem_object_checks/, /cp_ubsan_keep_addressof_call_p/, /cp_ubsan_fold_cast_call_p/, /p3100_constant_subscript_mem_ref|P3100_SUBSCRIPT_ADDRESS_P/, /cp_ubsan_maybe_check_aggr_init_call/, /wtd->loc|the expression most recently walked that has/
- gcc/cp/cp-tree.h : /CALL_EXPR_P3100_VIRTUAL_P|cp_ubsan_retarget_ptrmem_object_checks|cp_ubsan_maybe_check_unaccessed_|p3100_indirection_glvalue_p|cp_ubsan_maybe_check_address_of|cp_ubsan_keep_addressof_call_p|cp_ubsan_virtual_base_glvalue_p|cp_ubsan_maybe_check_empty_copy_source|cp_ubsan_fold_cast_call_p|cp_ubsan_maybe_check_vbase_conversion|cp_ubsan_maybe_check_aggr_init_call/
- gcc/cp/rtti.cc : @build_dynamic_cast_1
- gcc/cp/class.cc : /vcheck/
- gcc/cp/tree.cc : /CALL_EXPR_P3100_VIRTUAL_P/
- gcc/cp/cp-ubsan.cc : /cp_ubsan_ptrmem_|cp_ubsan_retarget_ptrmem_object_checks|the null check of the object pointer of OP->\*PM|cp_ubsan_maybe_check_unaccessed_|P3100: \*PTR_P is the operand of an indirection|p3100_vbase_offset_r|p3100_virtual_base_object|p3100_subobject_indirection|p3100_virtual_base_conversion_p|p3100_object_pointer|p3100_indirection_glvalue_p|cp_ubsan_maybe_check_address_of|cp_ubsan_keep_addressof_call_p|c-family\/c-ubsan\.h|p3100_cast_call_p|cp_ubsan_virtual_base_glvalue_p|cp_ubsan_maybe_check_empty_copy_source|cp_ubsan_fold_cast_call_p|p3100_virtual_base_of_p|p3100_vbase_conversion_object|p3100_replace_operand|p3100_checked_object_pointer_p|cp_ubsan_maybe_check_vbase_conversion|p3100_empty_base_of_p|cp_ubsan_maybe_instrument_ptrmemfunc_call|cp_ubsan_this_pointer_p|cp_ubsan_check_guarded_subscript|cp_ubsan_maybe_check_dynamic_type_read|p3100_pmf_vtable_read_r|p3100_check_pmf_vtable_read|p3100_check_object_pointer_access|p3100_pmf_vtable_cond_|cp_ubsan_maybe_check_devirtualized_call|p3100_check_comma_r|p3100_check_pmf_base_object|p3100_vptr_|p3100_guard_vptr_check|p3100_pointer_count|p3100_count_pointer_r/
- gcc/cp/typeck.cc : /cp_ubsan_maybe_check_unaccessed_|cp_ubsan_maybe_check_address_of|p3100_indirection_glvalue_p|P3100_SUBSCRIPT_ADDRESS_P|P3100_ARITH_ADDRESS_P|p3100_null_check_applies_p|ubsan_p3100_unguarded_sum|p3100_strip_saved_pointer|p3100_fold_element_offset|P3100: true if the null check of ub:expr\.unary\.dereference\.nullptr$|p3100_null_check_made_p|p3100_downcast|P3100_DOWNCAST_ADDRESS_P/
- gcc/cp/cvt.cc : /cp_ubsan_maybe_check_unaccessed_/
- gcc/cp/call.cc : /CALL_EXPR_P3100_VIRTUAL_P|cp_ubsan_maybe_check_unaccessed_|cp_ubsan_maybe_check_empty_copy_source|p3100_check|The bounds check alone keeps the source/
- gcc/cp/constexpr.cc : /a null check the front end built|p3100_fn/
- gcc/lto-streamer-in.cc : *
- gcc/fold-const.cc : /SANITIZE_ALIGNMENT|first operand's pointed type/
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-align-* : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-alignment-* : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-null-* : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-nullderef-* : *
- gcc/testsuite/g++.dg/ubsan/p3100-alignment-* : *
- gcc/testsuite/g++.dg/ubsan/p3100-null-* : *
- gcc/testsuite/g++.dg/ubsan/null-ptrmem-object-once.C : *
- gcc/testsuite/g++.dg/ubsan/open-bug-null-member-call-class-return.C : *
- gcc/testsuite/g++.dg/ubsan/open-bug-null-downcast-ref-twice.C : *
- gcc/testsuite/g++.dg/ubsan/null-downcast-ref-offset.C : *
- gcc/testsuite/g++.dg/ubsan/align-char-array.C : *
- gcc/testsuite/g++.dg/ubsan/p3100-align-class-copy-sanitize.C : *
- gcc/testsuite/g++.dg/ubsan/null-base-member.C : *
- gcc/testsuite/g++.dg/ubsan/null-trivial-assign-call.C : *
- gcc/ubsan.cc : @ubsan_expand_bounds_ifn, @resolve_implicit_ub_reaction, @build_implicit_ub_handler, @implicit_ub_handler_operands, /^build_implicit_ub_reaction_call \(tree fndecl, location_t loc,$/, @ubsan_expand_null_ifn, @instrument_mem_ref, @instrument_null, /ADDRESS_ONLY says MEM is only having its address taken/, @ubsan_ptrmem_derived_p, @ubsan_p3100_reference_pointer_p, @ubsan_p3100_this_pointer_p, @ubsan_disable_binding_check, @ubsan_p3100_disable_this_check, /^static void ubsan_p3100_disable_this_check \(gimple \*\);$/, @ubsan_note_ptrmem_object_checks, /ptrmem_checked_ptrs|collected by pass_ubsan before it instruments anything|ubsan_binding_check|ubsan_retargeted_binding_checks|ubsan_note_binding_checks|ubsan_retarget_binding_check/
