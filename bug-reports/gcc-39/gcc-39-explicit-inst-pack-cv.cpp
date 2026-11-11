// GCC-39 / PR126797: an explicit instantiation of a variadic function template
// is rejected when a template argument bound to the pack is cv-qualified.
//
// Not a contracts bug -- plain C++11, no contract, no pack indexing.  Rejected
// by gcc 13.4, trunk and this branch alike; accepted by Clang.
//
// Compile with: -std=c++11

template <typename... Ts> void f (Ts...) {}
template void f<const int> (const int);

// Control: the non-variadic form matches.
template <typename T> void g (T) {}
template void g<const int> (const int);
