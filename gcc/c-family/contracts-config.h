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

#include <cstring>

/* Contract assertion kinds.  Values match the C++ front-end's
   contract_assertion_kind enum.  */
enum contract_assertion_kind : unsigned short {
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
enum contract_evaluation_semantic : unsigned short {
  CES_INVALID = 0,
  CES_IGNORE = 1,
  CES_OBSERVE = 2,
  CES_ENFORCE = 3,
  CES_QUICK = 4,
  CES_ASSUME = 5,
  CES_NOEXCEPT_OBSERVE = 6,
  CES_NOEXCEPT_ENFORCE = 7,
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
    default: return "invalid";
    }
}

/* Linkage for a dynamic-selection function name (output.dynamic.linkage).  */
enum contract_dyn_linkage {
  CDL_CXX = 0,   /* "C++" (default): name may be qualified; compiler mangles */
  CDL_C   = 1    /* "C": name is the verbatim symbol (no mangling) */
};

/* Bitmask constant: all four standard semantics allowed.  The P3100
   "assume" semantic is deliberately excluded; it is only added to a
   query's allowed set when -fcontracts-allow-assume is in effect.  */
#define CES_ALL_ALLOWED \
  ((1 << CES_IGNORE) | (1 << CES_OBSERVE) \
   | (1 << CES_ENFORCE) | (1 << CES_QUICK))

/* Bitmask constant: every valid evaluation semantic, including "assume".
   This is the flag-independent "no restriction" set that a label's
   allowed_semantics facet narrows; the -fcontracts-allow-assume gate is
   applied separately, at query construction.  */
#define CES_ALL_ALLOWED_WITH_ASSUME \
  (CES_ALL_ALLOWED | (1 << CES_ASSUME))

/* Bitmask constant: every valid evaluation semantic, including "assume"
   and the D4298 noexcept-terminating variants.  This is the
   flag-independent "no restriction" set that a label's allowed_semantics
   facet narrows; the -fcontracts-allow-assume / -fcontracts-p4298 gates are
   applied separately, at query construction.  */
#define CES_ALL_ALLOWED_WITH_EXTENSIONS \
  (CES_ALL_ALLOWED_WITH_ASSUME \
   | (1 << CES_NOEXCEPT_ENFORCE) | (1 << CES_NOEXCEPT_OBSERVE))

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
struct contract_config_result {
  contract_evaluation_semantic semantic;  /* clamped default / weak fallback */
  const char *dyn_name;                    /* NULL => not dynamic */
  unsigned char dyn_linkage;               /* contract_dyn_linkage */
  bool dyn_provideweak;
  /* True when the matched entry is a P3595 dynamic one that deliberately
     requested no compile-time default ("dynamic" with no "semantic").
     SEMANTIC is then CES_INVALID because none was asked for, which is a
     different thing from resolution having failed.  */
  bool no_static_default;
};

/* Initialize the global config from command-line flags.
   Called eagerly from c_common_post_options when a configuration source
   is present, and otherwise lazily on the first contract_config_resolve
   call.  */
extern void contract_config_init (void);

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
   level.  Returns CES_INVALID only
   when MASK is empty of any representable semantic (an ill-formed configuration
   -- callers diagnose it).  Exposed so the P3595 dynamic-dispatch transform
   clamps returned selector values exactly as resolution does.  */
extern contract_evaluation_semantic
contract_semantic_best_fit (contract_evaluation_semantic candidate,
			    uint16_t mask);

#endif /* ! GCC_C_FAMILY_CONTRACTS_CONFIG_H */
