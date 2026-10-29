/* Shared configuration for contract evaluation semantics (P3595).

   Copyright (C) 2026 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef GCC_C_FAMILY_CONTRACTS_CONFIG_H
#define GCC_C_FAMILY_CONTRACTS_CONFIG_H

/* Contract assertion kinds, shared by the C and C++ front ends.  The values
   are part of the contracts ABI: they must stay equal to libcontracts'
   CXA_AK_* and <contracts.h>'s STDC_CONTRACT_* constants.  */
enum contract_assertion_kind : unsigned short
{
  CAK_INVALID = 0,
  CAK_PRE = 1,
  CAK_POST = 2,
  CAK_ASSERT = 3,
  CAK_MANUAL = 4,
  CAK_CASSERT = 5,
  CAK_POST_CAPTURE = 6,
  CAK_IMPLICIT = 7,	/* P3100 implicit contract assertion guarding UB.  */
};

/* Contract evaluation semantics.  Values match the
   -fcontract-evaluation-semantic= flag values.  */
enum contract_evaluation_semantic : unsigned short
{
  CES_INVALID = 0,
  CES_IGNORE = 1,
  CES_OBSERVE = 2,
  CES_ENFORCE = 3,
  CES_QUICK = 4,
  CES_ASSUME = 5,
  CES_NOEXCEPT_OBSERVE = 6,
  CES_NOEXCEPT_ENFORCE = 7,
  /* P4405: no run-time effect; an evaluation is diagnosed at compile time
     unless its predicate is proven -- an error for prove, a warning
     (-Wcontract-unproven) for diagnose.  */
  CES_PROVE = 8,
  CES_DIAGNOSE = 9,
  /* The largest valid semantic; masks of semantics are uint16_t.  */
  CES_LAST = CES_DIAGNOSE,
};

/* Parse a semantic name string (e.g. "observe") to its enum value, or
   CES_INVALID if NAME does not name a known semantic.  This is the single
   source of truth for semantic-name spelling; it is inline (rather than
   living in contracts-config.cc) so that gcc/opts.cc -- which is linked
   into every language's compiler proper, not just the ones with a
   c-family front end, and so cannot depend on c-family object code --
   can reuse it for -fsanitize-semantic= without hand-rolling a second
   table.  */
inline contract_evaluation_semantic
contract_semantic_from_name (const char *name)
{
  if (strcmp (name, "ignore") == 0) return CES_IGNORE;
  if (strcmp (name, "observe") == 0) return CES_OBSERVE;
  if (strcmp (name, "enforce") == 0) return CES_ENFORCE;
  if (strcmp (name, "quick_enforce") == 0) return CES_QUICK;
  if (strcmp (name, "assume") == 0) return CES_ASSUME;
  if (strcmp (name, "noexcept_enforce") == 0) return CES_NOEXCEPT_ENFORCE;
  if (strcmp (name, "noexcept_observe") == 0) return CES_NOEXCEPT_OBSERVE;
  if (strcmp (name, "prove") == 0) return CES_PROVE;
  if (strcmp (name, "diagnose") == 0) return CES_DIAGNOSE;
  return CES_INVALID;
}

/* The inverse of contract_semantic_from_name: the canonical spelling for
   a contract_evaluation_semantic value, or "invalid" for CES_INVALID /
   any other out-of-range value.  Single source of truth for semantic
   name spelling in diagnostics and debug output (e.g. the P3100
   -fsanitize-semantic= allowed-set errors and -fsanitize-semantic-print
   debug dump in gcc/opts.cc), kept next to contract_semantic_from_name
   for the same reason (opts.cc cannot depend on c-family object code).  */
inline const char *
contract_semantic_name (contract_evaluation_semantic semantic)
{
  switch (semantic)
    {
    case CES_IGNORE: return "ignore";
    case CES_OBSERVE: return "observe";
    case CES_ENFORCE: return "enforce";
    case CES_QUICK: return "quick_enforce";
    case CES_ASSUME: return "assume";
    case CES_NOEXCEPT_ENFORCE: return "noexcept_enforce";
    case CES_NOEXCEPT_OBSERVE: return "noexcept_observe";
    case CES_PROVE: return "prove";
    case CES_DIAGNOSE: return "diagnose";
    default: return "invalid";
    }
}

/* True if SEM is one of the noexcept semantics (P4298), which are
   reported through the _noexcept-suffixed entry points.  */
inline bool
contract_semantic_noexcept_p (contract_evaluation_semantic sem)
{
  return sem == CES_NOEXCEPT_ENFORCE || sem == CES_NOEXCEPT_OBSERVE;
}

/* True if SEM is one of the P4405 semantics, prove and diagnose: the
   contract stays active up to the point where its check would be emitted,
   but no check is emitted and the predicate is never evaluated; instead an
   evaluation that cannot be proven is diagnosed there.  */
inline bool
contract_semantic_proving_p (contract_evaluation_semantic sem)
{
  return sem == CES_PROVE || sem == CES_DIAGNOSE;
}

/* D4405: the analysis evaluation of a contract assertion -- its first, the
   only one prove or diagnose is ever applied to -- has one of ignore,
   diagnose and prove, ordered by strength: ignore < diagnose < prove.
   Return the stronger of A and B, each one of the three.  */
inline contract_evaluation_semantic
contract_analysis_stronger (contract_evaluation_semantic a,
			    contract_evaluation_semantic b)
{
  auto rank = [] (contract_evaluation_semantic s)
    { return s == CES_PROVE ? 2 : s == CES_DIAGNOSE ? 1 : 0; };
  return rank (b) > rank (a) ? b : a;
}

/* The bits of prove and diagnose in a mask of semantics.  */
#define CES_PROVING_MASK ((1 << CES_PROVE) | (1 << CES_DIAGNOSE))

/* D4405: the allowed set A of a contract assertion (MASK, the label's set
   intersected with the semantics the flags enable) splits into its runtime
   part R, the semantics other than prove and diagnose, and its analysis part
   N, those two.  The allowed set of the analysis evaluation is N, plus
   ignore when R is not empty (a runtime evaluation then has an allowed
   semantic).  */
inline uint16_t
contract_analysis_allowed_mask (uint16_t mask)
{
  uint16_t n = mask & CES_PROVING_MASK;
  if (mask & ~CES_PROVING_MASK)
    n |= (1 << CES_IGNORE);
  return n;
}

/* D4405: the allowed set of a runtime evaluation of a contract assertion
   whose allowed set is MASK.  A caller-side evaluation may always be ignore:
   the callee-side evaluation is made with an allowed semantic.  A
   callee-side one has R, or {ignore} when R is empty, since the analysis
   evaluation must then be prove or diagnose in every translation unit --
   decided by the labels alone, never by the configuration.  Zero when both
   R and N are empty, which is ill-formed.  (A constant evaluation is
   adjusted to MASK itself: contract_constant_evaluation_semantic.)  */
inline uint16_t
contract_runtime_allowed_mask (uint16_t mask, bool caller_side)
{
  uint16_t r = mask & ~CES_PROVING_MASK;
  if (caller_side || (!r && (mask & CES_PROVING_MASK)))
    r |= (1 << CES_IGNORE);
  return r;
}

/* D4405 (DECISIONS.md E142): constant evaluation is never analyzed, and a
   prove or diagnose it reaches -- from the configuration, a flag, or a
   label's allowed set or compute_semantic -- is not ignore: the one
   evaluation being made must satisfy a prove, as under enforce (a violation
   makes the expression not a constant expression), and a diagnose is
   observe (a violation is a warning).  Return the semantic a constant
   evaluation uses for SEM.  */
inline contract_evaluation_semantic
contract_constant_evaluation_semantic (contract_evaluation_semantic sem)
{
  if (sem == CES_PROVE)
    return CES_ENFORCE;
  if (sem == CES_DIAGNOSE)
    return CES_OBSERVE;
  return sem;
}

/* Linkage for a dynamic-selection function name (output.dynamic.linkage).  */
enum contract_dyn_linkage
{
  CDL_CXX = 0,   /* "C++" (default): name may be qualified; compiler mangles */
  CDL_C   = 1    /* "C": name is the verbatim symbol (no mangling) */
};

/* Bitmask constant: all four standard semantics allowed.  The P3100
   "assume" semantic is deliberately excluded; it is only added to a
   query's allowed set when -fcontracts-allow-assume is in effect.  */
#define CES_ALL_ALLOWED \
  ((1 << CES_IGNORE) | (1 << CES_OBSERVE) \
   | (1 << CES_ENFORCE) | (1 << CES_QUICK))

/* Bitmask constant: the four standard evaluation semantics plus "assume".
   The flag-independent "no restriction" set is
   CES_ALL_ALLOWED_WITH_EXTENSIONS, below, which also has the P4298
   variants.  */
#define CES_ALL_ALLOWED_WITH_ASSUME \
  (CES_ALL_ALLOWED | (1 << CES_ASSUME))

/* Bitmask constant: every valid evaluation semantic, including "assume",
   the P4298 noexcept-terminating variants and the P4405 prove and
   diagnose.  This is the flag-independent "no restriction" set that a
   label's allowed_semantics facet narrows; the -fcontracts-allow-assume /
   -fcontracts-p4298 / -fcontracts-p4405 gates are applied separately, at
   query construction.  */
#define CES_ALL_ALLOWED_WITH_EXTENSIONS \
  (CES_ALL_ALLOWED_WITH_ASSUME \
   | (1 << CES_NOEXCEPT_ENFORCE) | (1 << CES_NOEXCEPT_OBSERVE) \
   | (1 << CES_PROVE) | (1 << CES_DIAGNOSE))

/* The per-assertion query type is declared in contracts-config-query.h,
   and the parsed-config storage types are private to contracts-config.cc.
   Neither is exposed here so that restructuring them does not force a
   rebuild of everything that transitively includes cp/contracts.h.  */

struct contract_query;

/* The result of resolving a contract assertion's configuration.  A plain
   compile-time entry yields just SEMANTIC (with DYN_NAME == NULL).  A
   P3595 "dynamic" entry additionally yields the selector descriptor, and
   SEMANTIC is the compile-time default (the value the weak definition
   returns and the value used in constant evaluation).  */
struct contract_config_result
{
  /* RUNTIME, or CES_INVALID if either evaluation's resolution failed.  */
  contract_evaluation_semantic semantic;  /* clamped default */
  /* The matched entry's own "semantic", before it is clamped to the
     allowed set: what a P3595 dynamic selector's weak default returns.
     CES_INVALID when the entry gives none.  */
  contract_evaluation_semantic configured;
  const char *dyn_name;                    /* NULL => not dynamic */
  unsigned char dyn_linkage;               /* contract_dyn_linkage */
  bool dyn_provideweak;
  /* True when the matched entry is a P3595 dynamic one that deliberately
     requested no compile-time default ("dynamic" with no "semantic").
     SEMANTIC is then CES_INVALID because none was asked for, which is a
     different thing from resolution having failed.  */
  bool no_static_default;
  /* D4405: the semantic of the runtime evaluation queried, adjusted to its
     allowed set (contract_runtime_allowed_mask), and never prove or
     diagnose; CES_INVALID if resolution failed.  */
  contract_evaluation_semantic runtime;
  /* D4405: the semantic of the analysis evaluation -- ignore, diagnose or
     prove -- adjusted to its allowed set (contract_analysis_allowed_mask).
     For a callee-side query it is the assertion's; for a caller-side one
     only what the caller-side lookup moved into it; never anything but
     ignore in constant evaluation.  */
  contract_evaluation_semantic analysis;
};

/* Initialize the global config from command-line flags.
   Called eagerly from c_common_post_options when a configuration source
   is present, and otherwise lazily on the first contract_config_resolve
   call.  */
extern bool contract_config_names_semantic (contract_evaluation_semantic);
extern void contract_config_init (void);

/* A hash of the configuration parsed from the configuration sources, or 0
   if there are none; a precompiled header built with a different one is not
   valid.  */
extern hashval_t contract_config_hash (void);

/* Call FN once for each distinct name of a "C++"-linkage P3595 dynamic
   selector in the configuration.  */
extern void contract_config_for_each_cxx_selector (void (*) (const char *));

/* Resolve the configuration for a contract assertion.  */
extern contract_config_result
contract_config_resolve (const contract_query *query);

/* Find the best-fit evaluation semantic for CANDIDATE within the allowed set
   MASK.  Semantics are ranked by a safety level -- assume(0) < ignore(1) <
   observe/noexcept_observe(2) < enforce/noexcept_enforce(3) < quick_enforce(4).
   Starting at CANDIDATE's level the search prefers, in order: the same level,
   then the nearest safer (higher) level, then the safest available (nearest
   lower) level.  At the two-variant levels (2 and 3) it prefers the variant
   matching CANDIDATE's throwing-ness (a potentially-throwing CANDIDATE prefers
   observe/enforce; a non-throwing one prefers
   noexcept_observe/noexcept_enforce), falling back to the other variant at that
   level.  The P4405 prove and diagnose share ignore's level (none of the
   three has a run-time effect): at that level prove tries prove, diagnose,
   ignore; diagnose tries diagnose, ignore, prove; every other candidate
   tries ignore, diagnose, prove.  So every semantic in MASK is reachable.
   A CANDIDATE that is not a valid semantic yields CES_INVALID.  Otherwise
   CES_INVALID means MASK holds no semantic (an ill-formed configuration --
   callers diagnose it).  Exposed so the P3595 dynamic-dispatch transform
   clamps returned selector values exactly as resolution does.  */
extern contract_evaluation_semantic
contract_semantic_best_fit (contract_evaluation_semantic candidate,
			    uint16_t mask);

#endif /* ! GCC_C_FAMILY_CONTRACTS_CONFIG_H */
