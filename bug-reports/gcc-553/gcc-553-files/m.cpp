// g++ -std=c++26 -fmodules -fcontracts -fcontract-evaluation-semantic=observe -c m.cpp
//
// Module interface: an exported function template, a member of an exported
// class template, and an exported non-template function, each with a
// precondition.  The importer is main.cpp.
export module m;
export template <class T> T g1 (T x) pre (x > 0) { return x; }
export template <class T> struct TS { T k (T x) pre (x > 0) { return x; } };
export int nn (int x) pre (x > 0) { return x; }
