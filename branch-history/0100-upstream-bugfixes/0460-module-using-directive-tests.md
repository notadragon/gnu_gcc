---
id: 0460-module-using-directive-tests
subject: 'testsuite: using-directives from header units and the std modules'
depends: []
regenerates: []
fixes: []
---

## Rationale

Tests only; GCC already behaves as they expect.  A using-directive in a
header unit reaches its importers whether it sits at global scope or
inside a namespace, so both unqualified lookup and qualified lookup into
the enclosing namespace see the nominated one.  `import std;` and
`import std.compat;` carry the using-directive `<chrono>` declares in
`std::chrono`, so `using namespace std::chrono;` brings the chrono
literals into scope after either import, as it does after
`#include <chrono>`.

## Compile gap

None.  Pure test additions against upstream behaviour; they use no
contracts and nothing this branch adds.

## Contents

- gcc/testsuite/g++.dg/modules/using-directive-hu-1_a.H : *
- gcc/testsuite/g++.dg/modules/using-directive-hu-1_b.C : *
- gcc/testsuite/g++.dg/modules/std-module-chrono-literals.C : *
- gcc/testsuite/g++.dg/modules/std-compat-module-chrono-literals.C : *
