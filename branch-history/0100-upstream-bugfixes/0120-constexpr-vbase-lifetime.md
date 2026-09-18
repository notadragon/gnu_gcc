---
id: 0120-constexpr-vbase-lifetime
subject: 'c++: a derived-to-virtual-base conversion uses the object'
depends: []
regenerates: []
fixes: []
---

## Rationale

Not a contracts bug.  A derived-to-virtual-base conversion consults the
most-derived object's layout to compute the virtual-base offset, so it is a
use of the object: performing it outside the object's lifetime is UB and not
a constant expression.  A non-virtual base conversion uses a static offset
and does not access the object.  The test mirrors
`clang/test/SemaCXX/constexpr-vbase-lifetime.cpp`.

## Compile gap

None beyond a C++26 target, which the test already requires via
`dg-do compile { target c++26 }`.

## Contents

- gcc/testsuite/g++.dg/cpp2a/constexpr-vbase-lifetime.C : *
