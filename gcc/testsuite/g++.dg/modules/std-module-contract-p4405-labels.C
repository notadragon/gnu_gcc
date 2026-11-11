// "import std;" built with -fcontracts-p3400 -fcontracts-p4405 exports
// D4405's standard labels prove, diagnose, always_prove and always_diagnose
// (and their types), found unqualified in a label through the
// contract_control directive as with the #include path
// (contracts/cpp26/p4405-std-labels.C).

// { dg-additional-options "-fmodules --compile-std-module -fcontracts -fcontracts-p3400 -fcontracts-p4405" }
// { dg-do compile { target c++26 } }
// { dg-module-cmi std }
// { dg-module-cmi std.compat }
// { dg-module-cmi <bits/stdc++.h> }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

import std;

namespace L = std::contracts::labels;
using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

constexpr L::prove_t p = L::prove;
constexpr L::diagnose_t d = L::diagnose;
constexpr L::always_prove_t ap = L::always_prove;
constexpr L::always_diagnose_t ad = L::always_diagnose;
static_assert (p.analysis_semantic () == evaluation_semantic::prove);
static_assert (d.analysis_semantic () == evaluation_semantic::diagnose);
static_assert (ap.allowed_semantics
	       == evaluation_semantic_set {evaluation_semantic::prove});
static_assert (ad.allowed_semantics
	       == evaluation_semantic_set {evaluation_semantic::diagnose});
static_assert ((L::prove | L::diagnose).analysis_semantic ()
	       == evaluation_semantic::prove);

void f (int x) pre<always_prove> (true);
void g (int x) pre<diagnose> (x > 0);
void h (int x) pre<prove | always_diagnose> (x > 0);
