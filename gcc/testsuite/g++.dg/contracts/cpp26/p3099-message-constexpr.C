// P3099: Verify message appears in constexpr contract violation diagnostic.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3099" }

constexpr int f(int x) pre(x > 0, "must be positive") {  // { dg-error "contract predicate is false in constant expression \\(must be positive\\)" }
    return x;
}

constexpr int y = f(-1);
