/* Contracts C accessor functions (pure C).

   Copyright (C) 2026 Free Software Foundation, Inc.

   This file is part of the GNU Contracts Runtime Library (libcontracts).

   Libcontracts is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   Libcontracts is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

/* These implement the C accessor API for a contract_violation object.  Such
   an object has a single leading pointer to the head of the data-block chain
   (the same representation as std::contracts::contract_violation), so the C
   accessors read that chain directly via __cxa_find_field rather than calling
   into any C++ member function.  */

#include "contracts-abi.h"

/* A contract_violation object is { const __cxa_contract_data_block* chain; }.  */
static const __cxa_contract_data_block *
cv_chain (const void *cv)
{
  return *(const __cxa_contract_data_block *const *) cv;
}

static const void *
find1 (const __cxa_contract_data_block *chain, uint8_t id)
{
  uint8_t ids[1];
  ids[0] = id;
  return __cxa_find_field (chain, ids, 1);
}

static const __cxa_source_location *
cv_loc (const __cxa_contract_data_block *chain)
{
  return (const __cxa_source_location *) find1 (chain, CXA_FIELD_SOURCE_LOCATION);
}

static uint8_t
cv_u8 (const __cxa_contract_data_block *chain, uint8_t id, uint8_t dflt)
{
  const void *p = find1 (chain, id);
  return p ? *(const uint8_t *) p : dflt;
}

const char *
stdc_contract_violation_comment (const void *cv)
{
  const void *p = find1 (cv_chain (cv), CXA_FIELD_COMMENT);
  return p ? *(const char *const *) p : "";
}

const char *
stdc_contract_violation_file (const void *cv)
{
  const __cxa_source_location *loc = cv_loc (cv_chain (cv));
  return (loc && loc->file_name) ? loc->file_name : "";
}

const char *
stdc_contract_violation_function (const void *cv)
{
  const __cxa_source_location *loc = cv_loc (cv_chain (cv));
  return (loc && loc->function_name) ? loc->function_name : "";
}

unsigned
stdc_contract_violation_line (const void *cv)
{
  const __cxa_source_location *loc = cv_loc (cv_chain (cv));
  return loc ? loc->line : 0;
}

unsigned
stdc_contract_violation_column (const void *cv)
{
  const __cxa_source_location *loc = cv_loc (cv_chain (cv));
  return loc ? loc->column : 0;
}

int
stdc_contract_violation_kind (const void *cv)
{
  return (int) cv_u8 (cv_chain (cv), CXA_FIELD_ASSERTION_KIND,
		      CXA_AK_UNSPECIFIED);
}

int
stdc_contract_violation_semantic (const void *cv)
{
  return (int) cv_u8 (cv_chain (cv), CXA_FIELD_EVALUATION_SEMANTIC,
		      CXA_ES_UNSPECIFIED);
}

int
stdc_contract_violation_detection_mode (const void *cv)
{
  return (int) cv_u8 (cv_chain (cv), CXA_FIELD_DETECTION_MODE,
		      CXA_DM_UNSPECIFIED);
}

int
stdc_contract_violation_is_terminating (const void *cv)
{
  uint8_t s = cv_u8 (cv_chain (cv), CXA_FIELD_EVALUATION_SEMANTIC,
		     CXA_ES_UNSPECIFIED);
  return (s == CXA_ES_ENFORCE
	  || s == CXA_ES_QUICK_ENFORCE
	  || s == CXA_ES_NOEXCEPT_ENFORCE) ? 1 : 0;
}
