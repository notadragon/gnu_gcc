# GCC-867: routed signed-overflow checks of a `_BitInt`: lost or doubled

**Kind:** defect
**Status:** Open, this branch (routing); both causes are stock
**Affects:** measured 2026-10-17 on the install of the M88-GCC tree, C,
-O0 to -O3 and -flto, `-fsanitize=signed-integer-overflow
-fsanitize-semantic=signed-integer-overflow:noexcept_observe`
**Watch tests:** `gcc.dg/ubsan/open-bug-p3100-routed-bitint.c`
**Clang:** not measured (llvm_llvm-project task row from M88-GCC)

## Symptom

* A signed overflow of a `_BitInt` of more than 128 bits,
  `a + b` with `_BitInt(135)` operands, is not reported to the
  contract-violation handler: libubsan prints `runtime error: unsigned
  integer overflow: <unknown> + <unknown> cannot be represented in type
  '_BitInt(135)'`, as stock GCC's does, and that check is not routed.
* An overflowing multiplication of a `_BitInt(127)` is reported to the
  handler twice.

## Cause

The runtime cannot represent a type wider than 128 bits: its type
descriptor is unknown, which libubsan takes for an unsigned type, so the
report's ErrorType is UnsignedIntegerOverflow, a Clang-only check whose
routing slot GCC leaves stock (`RUC_UNSIGNED_INTEGER_OVERFLOW`, reserved
in `contracts-routed-checks.def`).  The lowered multiplication check
(`gimple-lower-bitint.cc`, `lower_mul_overflow`) calls
`__ubsan_handle_mul_overflow` twice for one overflow; stock libubsan
prints the second report of a location no more, while routing calls the
handler for each.  Found 2026-10-17 by M88-GCC writing GCC-858's _BitInt
test.  C only: C++ has no `_BitInt`.

## Expected

Each overflow reported once to the handler.
