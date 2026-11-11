# GCC-860: `vp[1].nv ()` with routed `-fsanitize=null` and only `.access`

**Kind:** divergence, accepted by the user 2026-10-17 (DECISIONS.md
E121 (a), as E115 (a) for Clang's CLANG-793)
**Status:** Accepted, this branch only
**Affects:** measured 2026-10-17 on the install of the M88-GCC tree, -O0
and -O2, `-fsanitize=null -fsanitize-semantic=null:noexcept_observe`
(with `-fsanitize=vptr` too) and only
`ub:expr.unary.dereference.nullptr.access` configured
**Tests:** `g++.dg/ubsan/p3100-null-vptr-sanitize-null-access*.C` pin it
(`sub 0`)
**Clang:** CLANG-793 (its pmf form, accepted as a divergence, E115 (a))

## Symptom

With a null `V *vp`, `vp[1].nv ()` -- a non-virtual call on an element at a
constant subscript, the `.indirection` leaf -- is not reported when only
`.access` is configured: the sanitizer, which makes the null check of the
leaf not configured (E111 (a)'s rule), tests `vp + 1`, stock behaviour,
not null.  GCC's pmf form `(hp[1].m.*p) ()` is reported there.  This is
E115's shape (Clang's `(hp[1].m.*p)()`) for a GCC member call; E115 (a)
was answered for Clang's form.

## History

Filed by M86-GCC with the double reports of `mp->W2::w2 ()` and
`static_cast<D &> (*vp).d` with routed `-fsanitize=null`; M88-GCC added
`mp->nw ()`, `hp->m.nv ()` and the non-polymorphic downcast read
(GCC-862, GCC-863) and, under DECISIONS.md E120, made each once
(`p3100-null-sanitize-null-once*.C`): the front end makes no
`-fsanitize=null` check of a member call's object pointer that is the
address of a subobject whose object pointer the ubsan pass checks, and the
ubsan pass none of a read through a reference just bound, whose binding is
checked.  With the group ignored the downcast forms stay twice, stock.

## Decision

Accepted (DECISIONS.md E121 (a), 2026-10-17), as E115 (a) did for Clang's
form: where the leaf is not configured, the null check is the sanitizer's,
and its stock check of `vp + 1` stands.
