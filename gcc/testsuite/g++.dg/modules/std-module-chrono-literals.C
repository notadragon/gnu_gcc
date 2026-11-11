// D3400R6 [std.modules]: "import std;" exports every using-directive the
// importable library headers provide, so the one <chrono> declares in
// std::chrono (using namespace literals::chrono_literals;) reaches the
// importer and "using namespace std::chrono;" brings the chrono literals
// into scope, as it does after #include <chrono> or import <chrono>;.

// { dg-additional-options "-fmodules --compile-std-module" }
// { dg-do compile { target c++26 } }
// { dg-module-cmi std }
// { dg-module-cmi std.compat }
// { dg-module-cmi <bits/stdc++.h> }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

import std;

using namespace std::chrono;
auto x = 1h;
static_assert (std::is_same_v<decltype (x), hours>);
