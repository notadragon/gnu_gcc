/* C-specific contract configuration (P3595).

   Provides the C implementation of contract_query::get_ns() and
   contract_query::get_caller_ns().  The shared config logic (parsing,
   resolution) is in c-family/contracts-config.cc.

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
#include "tree.h"
#include "c-family/contracts-config.h"
#include "c-family/contracts-config-query.h"

/* C implementation of contract_query::get_ns().
   C has no namespaces, so this always returns NULL.  */

const char *
contract_query::get_ns () const
{
  return NULL;
}

/* C implementation of contract_query::get_caller_ns().
   C has no namespaces, so this always returns NULL.  */

const char *
contract_query::get_caller_ns () const
{
  return NULL;
}
