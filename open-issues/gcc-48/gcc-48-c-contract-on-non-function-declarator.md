# GCC-48: The C front end accepts a contract on a declarator that is not a function

**Kind:** deferred
**Status:** Open
**Affects:** `-fcontracts-p4299`, the D4299 C contracts extension; a `_Pre` or
`_Post` written in a typedef declaration, or on a parameter of function type
**Workaround:** none needed for using the compiler; the constructs are
meaningless.  The cost is that a user who writes one is not told.

## Symptom

The C front end has the same family of holes the C++ one had, at two of the
same positions, and it is one notch softer at each:

```c
typedef int F (int) _Pre (1);              /* accepted, NO diagnostic */
void takes_fn (int bar () _Pre (1));       /* warning only, compiles */
void defines_fn (int bar () _Pre (1)) { }  /* warning only, compiles */
```

The typedef case is entirely silent.  The predicate is not merely dropped, it
is never analysed -- `typedef int F (int) _Pre (x > 0);` with no `x` in scope
produces nothing at all, where a parsed predicate would report an undeclared
identifier.

The parameter case IS reported, but as
`warning: contract on a parameter declarator is ignored [-Wattributes]`, so
the translation unit still compiles with status 0.

For contrast, and correctly:

```c
int obj _Pre (1);   /* error: expected '=', ',', ';', 'asm' or '__attribute__' */
```

## Why it is open

The C++ side of this family was closed on 2026-09-16 by
`1250-contract-on-non-function-declarator`, which refuses a contract on any
of five non-function declarators, and by
`0160-requires-clause-on-parameter` for the sibling specifier.  The C front
end was deliberately left alone in that pass (user, 2026-09-16) to keep it
bounded.

Two questions want deciding together rather than by accident:

* whether the typedef case should be an error, as it now is in C++;
* whether the parameter case should be **promoted from warning to error** to
  match.  Today C and C++ disagree about the same construct, and the C
  behaviour is the same accepts-invalid the C++ fix just removed.

## Where it is

Not the same code as the C++ fix -- the C front end carries its own
mechanism.  `c_parser_direct_declarator_inner` (`gcc/c/c-parser.cc`) parses
the specifier after any parameter list and pushes raw tokens onto the
file-static `pending_contracts` / `pending_contract_tokens`, to be replayed
after `store_parm_decls`.

The parameter case is already shielded and reported: the parameter-list parse
saves and restores those globals and warns about whatever is left behind,
which is where the `-Wattributes` warning comes from.

The typedef case rides out through `c_parser_declaration_or_fndef`'s
semicolon arm, which discards pending contracts because a non-defining
declaration -- a prototype -- is *supposed* to drop them under D4299.  A
typedef and a prototype are the same shape at that point.  The discriminator
is `specs->storage_class == csc_typedef`, which unlike the C++ parser IS in
hand there, so the C fix is the cheaper of the two.

**Hazard for whoever takes it.**  The K&R old-style-parameter loop performs
the same save/restore of those globals, and its recursive
`c_parser_declaration_or_fndef` calls hit that very semicolon arm once per
old-style parameter declaration, against a deliberately-empty vector.  A
check placed at the drain point fires there spuriously; it belongs where the
decl-specifiers and the declarator are both in hand.  Test against
`gcc/testsuite/gcc.dg/contracts/contracts-kandr-pre.c`.

Ours -- D4299 is this branch's extension, not upstream's -- so there is
nothing to file.

Measured 2026-09-16 against g++/gcc 17.0.0 20260914 carrying the C++ fixes.
