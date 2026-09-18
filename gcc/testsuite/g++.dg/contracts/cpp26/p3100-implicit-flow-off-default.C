// P3100: the implicit contract assertion guarding "falling off the end of a
// value-returning function" ({stmt.return.flow.off}) defaults to the "assume"
// evaluation semantic under -fcontracts-p3100.  "assume" is today's behavior:
// no check is emitted, the UB is preserved, and no diagnostic is produced.
// This test verifies the resolution hook does not misfire for the default.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }

int f(int x) {
  if (x > 0)
    return x;
  // Falls off the end when x <= 0: core-language UB, guarded by an implicit
  // contract assertion that defaults to "assume" -- no check, no diagnostic.
}

int main() { return 0; }
