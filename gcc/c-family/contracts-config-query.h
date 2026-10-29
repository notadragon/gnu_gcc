/* Per-assertion query type for contract configuration resolution (P3595).

   This type is the parameter to contract_config_resolve.  It is kept
   in a separate header from contracts-config.h (which only declares the
   stable enums and resolution API) so that changes to the query layout
   only recompile the few frontend translation units that actually build
   and resolve queries, rather than everything that transitively includes
   cp/contracts.h.

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

#ifndef GCC_C_FAMILY_CONTRACTS_CONFIG_QUERY_H
#define GCC_C_FAMILY_CONTRACTS_CONFIG_QUERY_H

#include "c-family/contracts-config.h"

/* Per-assertion query passed to contract_config_resolve.

   The kind field is set directly by the caller (not derived from
   tree codes) so the resolution logic is frontend-agnostic.

   get_ns() and get_caller_ns() are declared here but defined by each
   frontend:
   - C++ (cp/contracts-config.cc): lazily computed from fndecl (or the
     caller) using decl_namespace_context
   - C (c/c-parser.cc): NULL (C has no namespaces)  */

struct contract_query
{
  tree fndecl = NULL_TREE;
  int kind = 0;
  bool caller_side = false;
  bool in_constant_evaluation = false;
  tree caller_fndecl = NULL_TREE;
  uint16_t allowed_mask = 0;
  vec<const char *> *groups = NULL;
  location_t loc = UNKNOWN_LOCATION;

  location_t caller_loc = UNKNOWN_LOCATION;   /* Call-site location for
						 caller-side resolution.  */

  /* D4405: the semantic the assertion-control object's analysis_semantic
     facet gives the analysis evaluation, which takes the stronger of it and
     its configured semantic; ignore when there is none.  Read only for a
     callee-side query outside constant evaluation.  */
  contract_evaluation_semantic label_analysis = CES_IGNORE;

  const char *get_ns () const;
  const char *get_location_file () const;
  int get_location_line () const;
  const char *get_caller_location_file () const;
  int get_caller_location_line () const;
  const char *get_caller_ns () const;

private:
  mutable const char *cached_ns = NULL;
  mutable const char *cached_location_file = NULL;
  mutable int cached_location_line = 0;
  mutable bool ns_computed = false;
  mutable bool location_computed = false;

  /* Lazy caches for caller context.  */
  mutable const char *cached_caller_location_file = NULL;
  mutable int cached_caller_location_line = 0;
  mutable const char *cached_caller_ns = NULL;
  mutable bool caller_location_computed = false;
  mutable bool caller_ns_computed = false;
};

#endif /* ! GCC_C_FAMILY_CONTRACTS_CONFIG_QUERY_H */
