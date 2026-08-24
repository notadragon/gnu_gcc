/* C++-specific contract configuration (P3595).

   Provides the C++ implementation of contract_query::get_ns() and
   the build_namespace_string() helper.  The shared config logic
   (parsing, resolution) is in c-family/contracts-config.cc.

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

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "cp-tree.h"
#include "contracts-config.h"

/* Build the qualified namespace string from a namespace DECL by
   walking up to global_namespace.  Returns "" for global namespace.  */

const char *
build_namespace_string (tree ns_decl)
{
  if (!ns_decl || ns_decl == global_namespace)
    return "";

  auto_vec<const char *> parts;
  for (tree ns = ns_decl; ns && ns != global_namespace;
       ns = CP_DECL_CONTEXT (ns))
    {
      if (TREE_CODE (ns) != NAMESPACE_DECL)
	break;
      tree name = DECL_NAME (ns);
      if (name)
	parts.safe_push (IDENTIFIER_POINTER (name));
    }

  if (parts.is_empty ())
    return "";

  size_t len = 0;
  for (int i = parts.length () - 1; i >= 0; i--)
    {
      len += strlen (parts[i]);
      if (i > 0)
	len += 2;
    }

  char *result = XNEWVEC (char, len + 1);
  char *p = result;
  for (int i = parts.length () - 1; i >= 0; i--)
    {
      size_t slen = strlen (parts[i]);
      memcpy (p, parts[i], slen);
      p += slen;
      if (i > 0)
	{
	  *p++ = ':';
	  *p++ = ':';
	}
    }
  *p = '\0';
  return result;
}

/* C++ implementation of contract_query::get_ns().
   Lazily computes the namespace from fndecl using
   decl_namespace_context.  */

const char *
contract_query::get_ns () const
{
  if (!ns_computed)
    {
      ns_computed = true;
      tree ns_ctx = NULL_TREE;
      if (fndecl)
	ns_ctx = decl_namespace_context (fndecl);
      else
	ns_ctx = current_decl_namespace ();
      cached_ns = build_namespace_string (ns_ctx);
    }
  return cached_ns;
}

/* C++ implementation of contract_query::get_caller_ns().
   Lazily computes the namespace of the caller context: from caller_fndecl
   if known, otherwise from the current namespace at the call site.  */

const char *
contract_query::get_caller_ns () const
{
  if (!caller_ns_computed)
    {
      caller_ns_computed = true;
      tree ns_ctx = NULL_TREE;
      if (caller_fndecl)
	ns_ctx = decl_namespace_context (caller_fndecl);
      else
	ns_ctx = current_decl_namespace ();
      cached_caller_ns = build_namespace_string (ns_ctx);
    }
  return cached_caller_ns;
}
