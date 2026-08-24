// P3400: "import std;" must give the same contract_control short-name
// lookup that "#include <contracts>" does.
//
// <contracts> installs "using contract_control namespace
// std::contracts::labels;" for the include path.  std.cc.in exported the
// labels partition declaration by declaration but not that directive, so
// import std; users had to spell every label out in full.  The
// export-completeness check that produced the original export list walks
// std for public, non-uglified, non-deprecated *decls*, and a
// using-directive is not one -- which is why the omission was invisible
// to it.

// { dg-additional-options "-fmodules --compile-std-module -fcontracts-p3850" }
// { dg-do compile { target c++26 } }
// { dg-module-cmi std }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

import std;

// Qualified: guards the per-declaration exports.
void
g (int x) pre<std::contracts::labels::review> (x > 0)
{
}

// Unqualified: the directive itself.
void
h (int x) pre<review> (x > 0)
{
}
