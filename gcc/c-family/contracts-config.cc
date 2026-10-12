/* Shared configuration for contract evaluation semantics (P3595).

   This file is compiled into both cc1 (C) and cc1plus (C++) via
   C_COMMON_OBJS.  Frontend-specific behavior (namespace resolution)
   is provided by each frontend's implementation of
   contract_query::get_ns().

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

/* For the JSON headers (json-parsing.h uses std::map).  */
#define INCLUDE_MAP
#define INCLUDE_STRING
#define INCLUDE_VECTOR
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tree.h"
#include "options.h"
#include "c-family/contracts-config.h"
#include "c-family/contracts-config-query.h"
#include "diagnostic.h"
#include "json-parsing.h"
#include "json-diagnostic.h"
#include "c-family/contracts-config-source.h"

/* Parsed-config storage types.  These are private to this file: only the
   parsing and resolution logic below touches them, so keeping them out of
   the widely-included contracts-config.h means they can be restructured
   without triggering a rebuild of the whole C++ front end.  */

/* A line range for location matching.  */

struct contract_line_range
{
  int start;
  int end;
};

/* One rule in the ordered configuration list.  */

struct contract_config_entry
{
  int kind = -1;
  int caller_side = -1;
  int constexpr_eval = -1;
  const char *group = NULL;
  const char *ns = NULL;
  const char *location_file = NULL;
  vec<contract_line_range> *location_lines = NULL;
  int location_column = 0;		/* 0: any column.  */

  /* Caller-context match criteria (P3595 "caller" object).  NULL/absent
     for a plain "caller": true entry or a callee-side entry.  */
  const char *caller_ns = NULL;
  const char *caller_location_file = NULL;
  vec<contract_line_range> *caller_location_lines = NULL;
  int caller_location_column = 0;	/* 0: any column.  */

  contract_evaluation_semantic semantic = CES_INVALID;

  /* output.dynamic descriptor.  dyn_name == NULL means no dynamic
     selection was configured for this entry.  */
  char *dyn_name = NULL;
  unsigned char dyn_linkage = CDL_CXX;   /* contract_dyn_linkage */
  bool dyn_provideweak = true;
  bool has_semantic = false;             /* was "semantic" explicitly given?  */
};

/* Parse a contract kind name string to its enum value.  */

static int
parse_kind_name (const char *name)
{
  if (strcmp (name, "pre") == 0) return (int) CAK_PRE;
  if (strcmp (name, "post") == 0) return (int) CAK_POST;
  if (strcmp (name, "contract_assert") == 0) return (int) CAK_ASSERT;
  /* P3100: implicit contract assertions guarding core-language UB.  */
  if (strcmp (name, "implicit") == 0) return (int) CAK_IMPLICIT;
  return -2;  /* Unknown kind name (distinct from the -1 "unset" sentinel).  */
}

/* The safety level of a semantic: higher is "safer" (more checking).  Two
   semantics share a level when they differ only in throwing-ness.  */

static int
contract_semantic_level (contract_evaluation_semantic s)
{
  switch (s)
    {
    case CES_ASSUME:		return 0;
    case CES_IGNORE:		return 1;
    case CES_OBSERVE:
    case CES_NOEXCEPT_OBSERVE:	return 2;
    case CES_ENFORCE:
    case CES_NOEXCEPT_ENFORCE:	return 3;
    case CES_QUICK:		return 4;
    default:			return -1;
    }
}

/* Return the first semantic present in MASK at safety level LVL, trying the two
   variants (levels 2 and 3) in an order that prefers the throwing variant when
   PREFER_THROWING, else the noexcept one.  CES_INVALID if the level is empty in
   MASK.  */

static contract_evaluation_semantic
contract_semantic_at_level (int lvl, uint16_t mask, bool prefer_throwing)
{
  contract_evaluation_semantic a = CES_INVALID, b = CES_INVALID;
  switch (lvl)
    {
    case 0: a = CES_ASSUME; break;
    case 1: a = CES_IGNORE; break;
    case 2:
      a = prefer_throwing ? CES_OBSERVE : CES_NOEXCEPT_OBSERVE;
      b = prefer_throwing ? CES_NOEXCEPT_OBSERVE : CES_OBSERVE;
      break;
    case 3:
      a = prefer_throwing ? CES_ENFORCE : CES_NOEXCEPT_ENFORCE;
      b = prefer_throwing ? CES_NOEXCEPT_ENFORCE : CES_ENFORCE;
      break;
    case 4: a = CES_QUICK; break;
    default: return CES_INVALID;
    }
  if (a != CES_INVALID && (mask & (1 << a)))
    return a;
  if (b != CES_INVALID && (mask & (1 << b)))
    return b;
  return CES_INVALID;
}

/* See contracts-config.h.  Walk the safety levels from CANDIDATE's own level
   outward -- same level, then upward (nearest safer), then downward (safest
   available) -- returning the first semantic present in MASK, preferring at
   each two-variant level the variant matching CANDIDATE's throwing-ness.
   Assume is level 0, so the upward walk from it reaches ignore first.
   CES_INVALID means MASK admits no semantic.  */

contract_evaluation_semantic
contract_semantic_best_fit (contract_evaluation_semantic candidate,
			    uint16_t mask)
{
  int lc = contract_semantic_level (candidate);
  if (lc < 0)
    return CES_INVALID;
  bool prefer_throwing = (candidate == CES_OBSERVE || candidate == CES_ENFORCE);

  contract_evaluation_semantic r
    = contract_semantic_at_level (lc, mask, prefer_throwing);
  if (r != CES_INVALID)
    return r;
  for (int lvl = lc + 1; lvl <= 4; lvl++)
    if ((r = contract_semantic_at_level (lvl, mask, prefer_throwing))
	!= CES_INVALID)
      return r;
  for (int lvl = lc - 1; lvl >= 0; lvl--)
    if ((r = contract_semantic_at_level (lvl, mask, prefer_throwing))
	!= CES_INVALID)
      return r;
  return CES_INVALID;
}

/* The configuration of this translation unit: the entries of every
   configuration source, in order, then those derived from other options.
   Built on first use by contract_config_init.  */
static vec<contract_config_entry> global_config;
static bool global_config_initialized = false;

/* contract_query location accessors (frontend-agnostic).  get_ns() and
   get_caller_ns() are not defined here -- each frontend provides them.  */

const char *
contract_query::get_location_file () const
{
  if (!location_computed)
    {
      location_computed = true;
      if (loc != UNKNOWN_LOCATION)
	{
	  cached_location_file = LOCATION_FILE (loc);
	  cached_location_line = LOCATION_LINE (loc);
	}
      else
	{
	  cached_location_file = "";
	  cached_location_line = 0;
	}
    }
  return cached_location_file;
}

int
contract_query::get_location_line () const
{
  if (!location_computed)
    get_location_file ();
  return cached_location_line;
}

const char *
contract_query::get_caller_location_file () const
{
  if (!caller_location_computed)
    {
      caller_location_computed = true;
      if (caller_loc != UNKNOWN_LOCATION)
	{
	  cached_caller_location_file = LOCATION_FILE (caller_loc);
	  cached_caller_location_line = LOCATION_LINE (caller_loc);
	}
      else
	{
	  cached_caller_location_file = "";
	  cached_caller_location_line = 0;
	}
    }
  return cached_caller_location_file;
}

int
contract_query::get_caller_location_line () const
{
  if (!caller_location_computed)
    get_caller_location_file ();
  return cached_caller_location_line;
}

/* Return true if QUERY_NS starts with ENTRY_NS followed by "::" or end.  */

static bool
namespace_matches (const char *entry_ns, const char *query_ns)
{
  size_t elen = strlen (entry_ns);
  if (strncmp (query_ns, entry_ns, elen) != 0)
    return false;
  return query_ns[elen] == '\0' || (query_ns[elen] == ':'
				    && query_ns[elen + 1] == ':');
}

/* Return true if the LEN bytes at S form an identifier: a letter, `_' or
   `$' (or a byte of a UTF-8 sequence) followed by those or digits.  A
   dynamic selector's name is emitted as (or mangled into) a symbol, so
   anything else -- a template-id, an operator-function-id, a parameter
   list, a space -- would reach the assembler as garbage.  */

static bool
selector_identifier_p (const char *s, size_t len)
{
  if (len == 0)
    return false;
  for (size_t i = 0; i < len; ++i)
    {
      unsigned char c = s[i];
      if (c >= 0x80 || c == '_' || c == '$' || ISALPHA (c))
	continue;
      if (i > 0 && ISDIGIT (c))
	continue;
      return false;
    }
  return true;
}

/* Return true if NAME is a valid dynamic selector name for LINKAGE: a
   single identifier for C, identifiers separated by `::' for C++ (any
   leading `::' already removed).  */

static bool
selector_name_valid_p (const char *name, unsigned char linkage)
{
  if (linkage != CDL_CXX)
    return selector_identifier_p (name, strlen (name));
  for (;;)
    {
      const char *sep = strstr (name, "::");
      size_t len = sep ? (size_t) (sep - name) : strlen (name);
      if (!selector_identifier_p (name, len))
	return false;
      if (!sep)
	return true;
      name = sep + 2;
    }
}

/* Return true if QUERY_FILE ends with ENTRY_FILE (suffix match).  */

static bool
filename_suffix_matches (const char *entry_file, const char *query_file)
{
  size_t elen = strlen (entry_file);
  size_t qlen = strlen (query_file);
  if (elen > qlen)
    return false;
  if (strcmp (query_file + qlen - elen, entry_file) != 0)
    return false;
  if (elen == qlen)
    return true;
  char before = query_file[qlen - elen - 1];
  return before == '/' || before == '\\';
}

/* Parse "file" or "file:ranges", ranges being "N" or "N-M" separated by
   commas, into filename + optional line ranges.  On a malformed line-range
   list, diagnose against LOC_VAL and return NULL.  */

static const char *
parse_location_ranges (gcc_json_context &ctxt, json::value &loc_val,
		       const char *loc_str,
		       vec<contract_line_range> **line_ranges)
{
  const char *colon = strrchr (loc_str, ':');
  if (!colon || colon == loc_str)
    {
      *line_ranges = NULL;
      return xstrdup (loc_str);
    }

  if (!ISDIGIT (colon[1]))
    {
      *line_ranges = NULL;
      return xstrdup (loc_str);
    }

  const char *filename = xstrndup (loc_str, colon - loc_str);
  const char *ranges_str = colon + 1;

  *line_ranges = new vec<contract_line_range> ();

  bool ok = true;
  while (*ranges_str)
    {
      /* Each range is "N" or "N-M", ranges separated by ',".  Every
	 component must begin with a digit; strtol not advancing would
	 otherwise spin forever on stray input.  */
      if (!ISDIGIT (*ranges_str))
	{
	  ok = false;
	  break;
	}
      char *end;
      long start = strtol (ranges_str, &end, 10);
      long finish = start;
      ranges_str = end;
      if (*ranges_str == '-')
	{
	  ranges_str++;
	  if (!ISDIGIT (*ranges_str))
	    {
	      ok = false;
	      break;
	    }
	  finish = strtol (ranges_str, &end, 10);
	  ranges_str = end;
	}
      if (finish < start)
	{
	  ok = false;
	  break;
	}
      contract_line_range r;
      r.start = (int) start;
      r.end = (int) finish;
      (*line_ranges)->safe_push (r);

      if (*ranges_str == ',')
	ranges_str++;
      else if (*ranges_str != '\0')
	{
	  ok = false;
	  break;
	}
    }

  if (!ok)
    {
      json_error (ctxt, loc_val,
		  "malformed line range %qs in %<location%>", loc_str);
      delete *line_ranges;
      *line_ranges = NULL;
      free (const_cast<char *> (filename));
      return NULL;
    }

  return filename;
}

/* Parse a location string -- "file", "file:ranges" or "file:ranges:column"
   (P3595 allows a column "for the rare occasions where such precision is
   needed") -- into filename, optional line ranges and optional column (0
   when absent).  On a malformed line-range list, diagnose against LOC_VAL
   and return NULL.  */

static const char *
parse_location_string (gcc_json_context &ctxt, json::value &loc_val,
		       const char *loc_str,
		       vec<contract_line_range> **line_ranges, int *column)
{
  *column = 0;
  /* A trailing ":column" after a ":ranges" component.  (A file name ending
     in ":digits" is read as ranges, as before; a drive letter is followed
     by a separator, not a digit.)  */
  char *copy = NULL;
  const char *last = strrchr (loc_str, ':');
  if (last && last != loc_str && ISDIGIT (last[1])
      && last[1 + strspn (last + 1, "0123456789")] == '\0')
    {
      const char *prev = last - 1;
      while (prev > loc_str && *prev != ':')
	--prev;
      if (*prev == ':' && prev + 1 < last && ISDIGIT (prev[1])
	  && prev + 1 + strspn (prev + 1, "0123456789-,") == last)
	{
	  *column = atoi (last + 1);
	  copy = xstrndup (loc_str, last - loc_str);
	  loc_str = copy;
	}
    }

  const char *file = parse_location_ranges (ctxt, loc_val, loc_str,
					   line_ranges);
  free (copy);
  return file;
}

/* The keys each JSON object of a configuration entry may have; any other is
   warned about by warn_unknown_keys.  */
static const char *known_match_keys[] =
  { "kind", "group", "caller", "constexpr", "namespace", "location", NULL };

static const char *known_caller_keys[] =
  { "location", "namespace", NULL };

static const char *known_output_keys[] =
  { "semantic", "dynamic", NULL };

static const char *known_dynamic_keys[] =
  { "linkage", "name", "provideweak", NULL };

/* Warn about each key of OBJ, the configuration's OBJ_NAME object, that is
   not in the NULL-terminated KNOWN_KEYS.  Return true if there was one.  */

static bool
warn_unknown_keys (gcc_json_context &ctxt, const json::object *obj,
		   const char *obj_name, const char **known_keys)
{
  bool unknown = false;
  for (size_t i = 0; i < obj->get_num_keys (); i++)
    {
      const char *key = obj->get_key (i);
      bool found = false;
      for (const char **k = known_keys; *k; k++)
	if (strcmp (key, *k) == 0)
	  {
	    found = true;
	    break;
	  }
      if (!found)
	{
	  json_warning (ctxt, *obj->get (key), OPT_Wcontract_configuration,
			"unknown key %qs in %qs object; the entry is skipped",
			key, obj_name);
	  unknown = true;
	}
    }
  return unknown;
}

/* Parse the "dynamic" member DYN_VAL of an entry's "output" into E: the
   P3595 dynamic selector's name, linkage and provideweak.  Return false,
   after diagnosing it, if the entry is to be skipped.  */

static bool
parse_config_dynamic (gcc_json_context &ctxt, json::value *dyn_val,
		      contract_config_entry &e)
{
  json::object *dyn_obj = dyn_val->dyn_cast_object ();
  if (!dyn_obj)
    {
      json_error (ctxt, *dyn_val, "%<dynamic%> must be a JSON object");
      return false;
    }

  json::value *name_val = dyn_obj->get ("name");
  json::string *name_str
    = name_val ? name_val->dyn_cast_string () : NULL;
  if (!name_str)
    {
      json_error (ctxt, *dyn_val,
		  "%<dynamic%> requires a string %<name%>");
      return false;
    }
  e.dyn_name = xstrdup (name_str->get_string ());

  json::value *linkage_val = dyn_obj->get ("linkage");
  if (linkage_val)
    {
      json::string *linkage_str = linkage_val->dyn_cast_string ();
      const char *linkage_name
	= linkage_str ? linkage_str->get_string () : "";
      if (strcmp (linkage_name, "C++") == 0)
	e.dyn_linkage = CDL_CXX;
      else if (strcmp (linkage_name, "C") == 0)
	e.dyn_linkage = CDL_C;
      else
	{
	  json_error (ctxt, *linkage_val,
		      "%<linkage%> must be %<C%> or %<C++%>");
	  return false;
	}
    }

  /* A "C++" name is a qualified name: a leading `::' names the global
     namespace and is dropped, and every component must then be an
     identifier.  A "C" name is a single identifier.  */
  if (e.dyn_linkage == CDL_CXX && startswith (e.dyn_name, "::"))
    {
      char *stripped = xstrdup (e.dyn_name + 2);
      free (e.dyn_name);
      e.dyn_name = stripped;
    }
  if (!selector_name_valid_p (e.dyn_name, e.dyn_linkage))
    {
      if (e.dyn_linkage == CDL_CXX)
	json_error (ctxt, *name_val,
		    "%<name%> in %<dynamic%> is not a valid "
		    "qualified name");
      else
	json_error (ctxt, *name_val,
		    "%<name%> in %<dynamic%> is not a valid "
		    "identifier");
      return false;
    }

  json::value *pw_val = dyn_obj->get ("provideweak");
  bool pw_specified = false;
  if (pw_val)
    {
      pw_specified = true;
      if (pw_val->get_kind () == json::JSON_TRUE)
	e.dyn_provideweak = true;
      else if (pw_val->get_kind () == json::JSON_FALSE)
	e.dyn_provideweak = false;
      else
	{
	  json_error (ctxt, *pw_val,
		      "%<provideweak%> must be a boolean");
	  return false;
	}
    }

  /* semantic/provideweak interaction (P3595): provideweak needs a
     value for the weak definition to return, which comes from
     "semantic".  With no "semantic", an explicit "provideweak":
     true has nothing to return, so it is an error; otherwise
     provideweak defaults to false (no weak definition is emitted;
     the user must supply the selector themselves).  */
  if (!e.has_semantic)
    {
      if (pw_specified && e.dyn_provideweak)
	{
	  json_error (ctxt, *dyn_val,
		      "%<provideweak%> requires an output "
		      "%<semantic%>");
	  return false;
	}
      e.dyn_provideweak = false;
    }

  if (warn_unknown_keys (ctxt, dyn_obj, "dynamic", known_dynamic_keys))
    return false;
  return true;
}

/* Parse an entry's "output" member OUTPUT_OBJ into E: a compile-time
   semantic, a P3595 dynamic selector, or both.  Return false, after
   diagnosing it, if the entry is to be skipped.  */

static bool
parse_config_output (gcc_json_context &ctxt, json::object *output_obj,
		     contract_config_entry &e)
{
  json::value *sem_val = output_obj->get ("semantic");
  if (sem_val)
    {
      json::string *sem_str = sem_val->dyn_cast_string ();
      if (!sem_str)
	{
	  json_error (ctxt, *sem_val, "%<semantic%> must be a string");
	  return false;
	}
      contract_evaluation_semantic sem
	= contract_semantic_from_name (sem_str->get_string ());
      if (sem == CES_INVALID)
	{
	  json_error (ctxt, *sem_val,
		      "invalid contract evaluation semantic %qs",
		      sem_str->get_string ());
	  return false;
	}
      e.semantic = sem;
      e.has_semantic = true;
    }

  json::value *dyn_val = output_obj->get ("dynamic");
  if (dyn_val && !parse_config_dynamic (ctxt, dyn_val, e))
    return false;

  /* An "output" must select a semantic somehow: either a compile-time
     "semantic" or a "dynamic" selector (which may also carry a
     "semantic" default).  An empty output is a malformed config.  */
  if (!e.has_semantic && !e.dyn_name)
    {
      json_error (ctxt, *output_obj,
		  "%<output%> requires a %<semantic%> or a %<dynamic%> "
		  "field");
      return false;
    }

  if (warn_unknown_keys (ctxt, output_obj, "output", known_output_keys))
    return false;

  return true;
}

/* Parse the "caller" member CALLER_VAL of an entry's "match" into E: true,
   false, or an object of caller-context criteria.  Return false, after
   diagnosing it, if the entry is to be skipped.  */

static bool
parse_config_caller (gcc_json_context &ctxt, json::value *caller_val,
		     contract_config_entry &e)
{
  if (caller_val->get_kind () == json::JSON_TRUE)
    e.caller_side = 1;
  else if (caller_val->get_kind () == json::JSON_FALSE)
    e.caller_side = 0;
  else if (json::object *caller_obj
	     = caller_val->dyn_cast_object ())
    {
      e.caller_side = 1;
      json::value *cloc = caller_obj->get ("location");
      if (cloc)
	{
	  json::string *cloc_str = cloc->dyn_cast_string ();
	  if (!cloc_str)
	    {
	      json_error (ctxt, *cloc, "%<location%> in %<caller%> "
			  "must be a string");
	      return false;
	    }
	  e.caller_location_file
	    = parse_location_string (ctxt, *cloc,
				     cloc_str->get_string (),
				     &e.caller_location_lines,
				     &e.caller_location_column);
	  if (!e.caller_location_file)
	    return false;
	}
      json::value *cns = caller_obj->get ("namespace");
      if (cns)
	{
	  json::string *cns_str = cns->dyn_cast_string ();
	  if (!cns_str)
	    {
	      json_error (ctxt, *cns, "%<namespace%> in %<caller%> "
			  "must be a string");
	      return false;
	    }
	  if (!*cns_str->get_string ())
	    {
	      json_error (ctxt, *cns, "%<namespace%> in %<caller%> "
			  "must not be empty");
	      return false;
	    }
	  e.caller_ns = xstrdup (cns_str->get_string ());
	}
      if (warn_unknown_keys (ctxt, caller_obj, "caller",
			     known_caller_keys))
	return false;
    }
  else
    {
      json_error (ctxt, *caller_val,
		  "%<caller%> must be a boolean or object");
      return false;
    }
  return true;
}

/* Parse the "match" member MATCH_VAL of an entry into E: the criteria
   that select the contract assertions the entry applies to.  Return false,
   after diagnosing it, if the entry is to be skipped.  */

static bool
parse_config_match (gcc_json_context &ctxt, json::value *match_val,
		    contract_config_entry &e)
{
  json::object *match_obj = match_val->dyn_cast_object ();
  if (!match_obj)
    {
      json_error (ctxt, *match_val, "%<match%> must be a JSON object");
      return false;
    }

  json::value *kind_val = match_obj->get ("kind");
  if (kind_val)
    {
      json::string *kind_str = kind_val->dyn_cast_string ();
      if (!kind_str)
	{
	  json_error (ctxt, *kind_val, "%<kind%> must be a string");
	  return false;
	}
      int k = parse_kind_name (kind_str->get_string ());
      if (k == -2)
	{
	  json_error (ctxt, *kind_val, "invalid contract kind %qs",
		      kind_str->get_string ());
	  return false;
	}
      e.kind = k;
    }

  json::value *group_val = match_obj->get ("group");
  if (group_val)
    {
      json::string *group_str = group_val->dyn_cast_string ();
      if (!group_str)
	{
	  json_error (ctxt, *group_val, "%<group%> must be a string");
	  return false;
	}
      e.group = xstrdup (group_str->get_string ());
    }

  json::value *caller_val = match_obj->get ("caller");
  if (caller_val && !parse_config_caller (ctxt, caller_val, e))
    return false;

  json::value *ce_val = match_obj->get ("constexpr");
  if (ce_val)
    {
      if (ce_val->get_kind () == json::JSON_TRUE)
	e.constexpr_eval = 1;
      else if (ce_val->get_kind () == json::JSON_FALSE)
	e.constexpr_eval = 0;
      else
	{
	  json_error (ctxt, *ce_val, "%<constexpr%> must be a boolean");
	  return false;
	}
    }

  json::value *ns_val = match_obj->get ("namespace");
  if (ns_val)
    {
      json::string *ns_str = ns_val->dyn_cast_string ();
      if (!ns_str)
	{
	  json_error (ctxt, *ns_val, "%<namespace%> must be a string");
	  return false;
	}
      if (!*ns_str->get_string ())
	{
	  json_error (ctxt, *ns_val, "%<namespace%> must not be empty");
	  return false;
	}
      e.ns = xstrdup (ns_str->get_string ());
    }

  json::value *loc_val = match_obj->get ("location");
  if (loc_val)
    {
      json::string *loc_str = loc_val->dyn_cast_string ();
      if (!loc_str)
	{
	  json_error (ctxt, *loc_val, "%<location%> must be a string");
	  return false;
	}
      e.location_file
	= parse_location_string (ctxt, *loc_val, loc_str->get_string (),
				 &e.location_lines,
				 &e.location_column);
      if (!e.location_file)
	return false;
    }

  if (warn_unknown_keys (ctxt, match_obj, "match", known_match_keys))
    return false;
  return true;
}

/* Parse a JSON text and append config entries to global_config.  */

static void
parse_contract_config_json (const char *json_text, const char *source_desc)
{
  gcc_json_context ctxt (source_desc);
  json::parser_result_t result
    = json::parse_utf8_string (json_text, true, &ctxt);

  if (auto err = result.m_err.get ())
    {
      location_t loc = ctxt.make_location_for_range (err->get_range ());
      error_at (loc, "invalid JSON in contract configuration: %s",
		err->get_msg ());
      return;
    }

  json::value *root = result.m_val.get ();
  json::array *arr = root->dyn_cast_array ();
  if (!arr)
    {
      json_error (ctxt, *root, "contract configuration must be a JSON array");
      return;
    }

  for (int i = 0; i < (int) arr->size (); i++)
    {
      json::value *elem = arr->get (i);
      json::object *entry_obj = elem->dyn_cast_object ();
      if (!entry_obj)
	{
	  json_error (ctxt, *elem,
		      "contract configuration entry must be a JSON object");
	  continue;
	}

      json::value *output_val = entry_obj->get ("output");
      if (!output_val)
	{
	  json_error (ctxt, *entry_obj, "contract configuration entry is "
		      "missing required %<output%> field");
	  continue;
	}
      json::object *output_obj = output_val->dyn_cast_object ();
      if (!output_obj)
	{
	  json_error (ctxt, *output_val, "%<output%> must be a JSON object");
	  continue;
	}

      contract_config_entry e;

      if (!parse_config_output (ctxt, output_obj, e))
	continue;

      json::value *match_val = entry_obj->get ("match");
      if (match_val && !parse_config_match (ctxt, match_val, e))
	continue;

      global_config.safe_push (e);
    }
}

/* Parse the JSON configuration file PATH into global_config.  */

static void
parse_contract_config_file (const char *path)
{
  FILE *f = fopen (path, "r");
  if (!f)
    {
      error ("cannot open contract configuration file %qs: %m", path);
      return;
    }

  auto_vec<char> buf;
  char chunk[4096];
  size_t n;
  while ((n = fread (chunk, 1, sizeof (chunk), f)) > 0)
    for (size_t j = 0; j < n; j++)
      buf.safe_push (chunk[j]);

  if (!feof (f))
    {
      error ("error reading contract configuration file %qs: %m", path);
      fclose (f);
      return;
    }
  fclose (f);

  buf.safe_push ('\0');
  parse_contract_config_json (buf.address (), path);
}

/* Whether the LEN characters at NAME are a valid contract group name: made
   of letters, digits, `_', `-', `.' and `:' (P3400 names are dot-separated,
   P3100 names begin `ub:'), with no empty dot-separated component.  */

static bool
contract_group_name_valid_p (const char *name, size_t len)
{
  if (len == 0 || name[0] == '.' || name[len - 1] == '.')
    return false;
  for (size_t i = 0; i < len; ++i)
    if ((!ISALNUM (name[i]) && !strchr ("_-.:", name[i]))
	|| (name[i] == '.' && i + 1 < len && name[i + 1] == '.'))
      return false;
  return true;
}

/* Build global_config from the configuration sources, in command-line
   order, followed by the entries derived from -fcontract-evaluation-semantic=
   and related options.  */

void
contract_config_init (void)
{
  global_config.truncate (0);

  unsigned i;
  contract_config_source *src;
  FOR_EACH_VEC_ELT (contracts_config_sources, i, src)
    {
      switch (src->kind)
	{
	case CCSK_GROUP_SEMANTIC:
	  /* A comma-separated list of group:semantic elements, one entry
	     each, in order.  */
	  for (const char *elt = src->arg; ; )
	    {
	      const char *comma = strchr (elt, ',');
	      char *arg = comma ? xstrndup (elt, comma - elt) : xstrdup (elt);
	      /* The semantic follows the last colon: a P3100 group name
		 (ub:...) contains one itself.  */
	      const char *colon = strrchr (arg, ':');
	      if (!colon || colon == arg || colon[1] == '\0')
		error ("invalid %<-fcontract-group-evaluation-semantic=%> "
		       "argument %qs; expected %<group:semantic%>", arg);
	      else if (!contract_group_name_valid_p (arg, colon - arg))
		error ("invalid group name %qs in "
		       "%<-fcontract-group-evaluation-semantic=%> argument",
		       xstrndup (arg, colon - arg));
	      else
		{
		  contract_evaluation_semantic sem
		    = contract_semantic_from_name (colon + 1);
		  if (sem == CES_INVALID)
		    error ("invalid semantic %qs in "
			   "%<-fcontract-group-evaluation-semantic=%> argument",
			   colon + 1);
		  else
		    {
		      contract_config_entry e;
		      e.caller_side = 0;
		      e.group = xstrndup (arg, colon - arg);
		      e.semantic = sem;
		      e.has_semantic = true;
		      global_config.safe_push (e);
		    }
		}
	      free (arg);
	      if (!comma)
		break;
	      elt = comma + 1;
	    }
	  break;

	case CCSK_JSON_INLINE:
	  parse_contract_config_json (src->arg, "<command-line>");
	  break;

	case CCSK_JSON_FILE:
	  parse_contract_config_file (src->arg);
	  break;
	}
    }

  if (flag_contract_client_check == 0)
    {
      contract_config_entry e;
      e.caller_side = 1;
      e.semantic = CES_IGNORE;
      e.has_semantic = true;
      global_config.safe_push (e);
    }
  else if (flag_contract_client_check == 1)
    {
      contract_config_entry e;
      e.kind = CAK_POST;
      e.caller_side = 1;
      e.semantic = CES_IGNORE;
      e.has_semantic = true;
      global_config.safe_push (e);
    }

  if (!flag_contracts_definition_check)
    {
      contract_config_entry pre_e;
      pre_e.kind = (int) CAK_PRE;
      pre_e.caller_side = 0;
      pre_e.semantic = CES_IGNORE;
      pre_e.has_semantic = true;
      global_config.safe_push (pre_e);

      contract_config_entry post_e;
      post_e.kind = (int) CAK_POST;
      post_e.caller_side = 0;
      post_e.semantic = CES_IGNORE;
      post_e.has_semantic = true;
      global_config.safe_push (post_e);
    }

  /* P3100: implicit contract assertions guarding core-language UB default to
     the "assume" semantic -- i.e. today's behaviour (no check emitted; the UB
     remains and the optimizer may exploit it).  This builtin entry is pushed
     after any user-provided sources (so a user config can still override it,
     first-match-wins) but before the global -fcontract-evaluation-semantic
     catch-all, so implicit assertions do not pick up the global default (which
     is "enforce").  Gated on -fcontracts-p3100.  Unlike the explicit-contract
     "assume" semantic, this default is not gated on -fcontracts-allow-assume:
     it introduces no new UB (it is the status quo), so it is always available
     for implicit assertions once the feature is enabled.  */
  if (flag_contracts_p3100)
    {
      contract_config_entry e;
      e.kind = (int) CAK_IMPLICIT;
      e.semantic = CES_ASSUME;
      e.has_semantic = true;
      global_config.safe_push (e);
    }

  {
    contract_config_entry e;
    e.semantic
      = (contract_evaluation_semantic) flag_contract_evaluation_semantic;
    e.has_semantic = true;
    global_config.safe_push (e);
  }

  global_config_initialized = true;
}

/* A hash of the configuration parsed from the configuration sources, or 0
   if there are none.  The entries derived from other options are part of it;
   the entries, not the source text, are hashed, so equivalent sources
   agree.  */

hashval_t
contract_config_hash (void)
{
  if (!global_config_initialized || contracts_config_sources.is_empty ())
    return 0;

  inchash::hash h;
  auto add_str = [&h] (const char *s)
    {
      h.add_flag (s != NULL);
      if (s)
	h.add (s, strlen (s));
    };
  auto add_lines = [&h] (const vec<contract_line_range> *lines)
    {
      h.add_flag (lines != NULL);
      if (lines)
	for (const contract_line_range &r : *lines)
	  {
	    h.add_int (r.start);
	    h.add_int (r.end);
	  }
    };
  for (const contract_config_entry &e : global_config)
    {
      h.add_int (e.kind);
      h.add_int (e.caller_side);
      h.add_int (e.constexpr_eval);
      add_str (e.group);
      add_str (e.ns);
      add_str (e.location_file);
      add_lines (e.location_lines);
      h.add_int (e.location_column);
      add_str (e.caller_ns);
      add_str (e.caller_location_file);
      add_lines (e.caller_location_lines);
      h.add_int (e.caller_location_column);
      h.add_int (e.semantic);
      add_str (e.dyn_name);
      h.add_int (e.dyn_linkage);
      h.add_flag (e.dyn_provideweak);
      h.add_flag (e.has_semantic);
      h.commit_flag ();
    }
  hashval_t r = h.end ();
  return r ? r : 1;
}

/* True if some parsed configuration entry asks for SEM as its semantic.  */

bool
contract_config_names_semantic (contract_evaluation_semantic sem)
{
  for (const contract_config_entry &e : global_config)
    if (e.has_semantic && e.semantic == sem)
      return true;
  return false;
}

/* Call FN once for each distinct name of a "C++"-linkage P3595 dynamic
   selector in the parsed configuration, in configuration order.  Only a
   configuration source can name a selector, and one is parsed eagerly
   (c_common_post_options), so there is nothing to visit before then.  */

void
contract_config_for_each_cxx_selector (void (*fn) (const char *))
{
  unsigned i;
  contract_config_entry *e;
  FOR_EACH_VEC_ELT (global_config, i, e)
    {
      if (!e->dyn_name || e->dyn_linkage != CDL_CXX)
	continue;
      bool seen = false;
      for (unsigned j = 0; j < i && !seen; ++j)
	{
	  const contract_config_entry &prev = global_config[j];
	  seen = (prev.dyn_name && prev.dyn_linkage == CDL_CXX
		  && strcmp (prev.dyn_name, e->dyn_name) == 0);
	}
      if (!seen)
	fn (e->dyn_name);
    }
}

/* Return true if ENTRY matches QUERY.
   Kind matching uses query->kind directly (set by the frontend).  */

static bool
config_entry_matches (const contract_config_entry *entry,
		      const contract_query *query)
{
  if (entry->kind != -1 && entry->kind != query->kind)
    return false;

  /* An entry with no "caller" key matches callee-side queries only.  */
  if (entry->caller_side == -1)
    {
      if (query->caller_side)
	return false;
    }
  else if ((entry->caller_side == 1) != query->caller_side)
    return false;

  if (entry->constexpr_eval != -1
      && entry->constexpr_eval != (int) query->in_constant_evaluation)
    return false;

  if (entry->group != NULL)
    {
      if (!query->groups)
	return false;
      bool found = false;
      unsigned j;
      const char *qgroup;
      FOR_EACH_VEC_ELT (*query->groups, j, qgroup)
	{
	  size_t elen = strlen (entry->group);
	  if (strncmp (qgroup, entry->group, elen) == 0
	      && (qgroup[elen] == '\0' || qgroup[elen] == '.'))
	    {
	      found = true;
	      break;
	    }
	}
      if (!found)
	return false;
    }

  if (entry->ns != NULL)
    {
      const char *qns = query->get_ns ();
      if (!qns || !namespace_matches (entry->ns, qns))
	return false;
    }

  if (entry->location_file != NULL)
    {
      const char *qfile = query->get_location_file ();
      if (!qfile || !filename_suffix_matches (entry->location_file, qfile))
	return false;
      if (entry->location_lines)
	{
	  int qline = query->get_location_line ();
	  bool in_range = false;
	  unsigned j;
	  contract_line_range *r;
	  FOR_EACH_VEC_ELT (*entry->location_lines, j, r)
	    {
	      if (qline >= r->start && qline <= r->end)
		{
		  in_range = true;
		  break;
		}
	    }
	  if (!in_range)
	    return false;
	}
      if (entry->location_column
	  && LOCATION_COLUMN (query->loc) != entry->location_column)
	return false;
    }

  if (query->caller_side)
    {
      if (entry->caller_location_file != NULL)
	{
	  const char *cfile = query->get_caller_location_file ();
	  if (!cfile
	      || !filename_suffix_matches (entry->caller_location_file, cfile))
	    return false;
	  if (entry->caller_location_lines)
	    {
	      int cline = query->get_caller_location_line ();
	      bool in_range = false;
	      unsigned j;
	      contract_line_range *r;
	      FOR_EACH_VEC_ELT (*entry->caller_location_lines, j, r)
		if (cline >= r->start && cline <= r->end)
		  {
		    in_range = true;
		    break;
		  }
	      if (!in_range)
		return false;
	    }
	  if (entry->caller_location_column
	      && LOCATION_COLUMN (query->caller_loc)
		 != entry->caller_location_column)
	    return false;
	}
      if (entry->caller_ns != NULL)
	{
	  const char *cns = query->get_caller_ns ();
	  if (!cns || !namespace_matches (entry->caller_ns, cns))
	    return false;
	}
    }

  return true;
}

/* Resolve QUERY against global_config: the first matching entry's semantic,
   clamped to QUERY's allowed set, and its dynamic selector, if any.  */

contract_config_result
contract_config_resolve (const contract_query *query)
{
  if (!global_config_initialized)
    contract_config_init ();

  contract_config_result r = { CES_INVALID, NULL, CDL_CXX, false, false };

  contract_config_entry *entry = NULL;
  unsigned i;
  contract_config_entry *e;
  FOR_EACH_VEC_ELT (global_config, i, e)
    {
      if (!config_entry_matches (e, query))
	continue;

      /* P3595 (spec 3): in constant evaluation, a dynamic entry with no
	 compile-time "semantic" has nothing to return without calling the
	 selector (which is forbidden at compile time), so treat it as
	 non-matching and keep scanning to the next entry.  */
      if (query->in_constant_evaluation && e->dyn_name && !e->has_semantic)
	continue;

      entry = e;
      break;
    }

  /* No entry matched.  Caller-side checking is opt-in, so an unmatched
     caller-side query is ignore; an unmatched callee-side query has no
     semantic, which the caller diagnoses.  */
  if (!entry)
    {
      r.semantic = query->caller_side ? CES_IGNORE : CES_INVALID;
      return r;
    }

  r.semantic = contract_semantic_best_fit (entry->semantic,
					   query->allowed_mask);

  /* The dynamic descriptor is only meaningful at run time; a dynamic
     function is never called during constant evaluation (spec 3).  */
  if (!query->in_constant_evaluation && entry->dyn_name)
    {
      r.dyn_name = entry->dyn_name;
      r.dyn_linkage = entry->dyn_linkage;
      r.dyn_provideweak = entry->dyn_provideweak;
      r.no_static_default = !entry->has_semantic;
    }

  return r;
}
