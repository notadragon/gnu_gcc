/* Types for the ordered contract configuration source list.

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

#ifndef GCC_C_FAMILY_CONTRACTS_CONFIG_SOURCE_H
#define GCC_C_FAMILY_CONTRACTS_CONFIG_SOURCE_H

enum contract_config_source_kind {
  CCSK_GROUP_SEMANTIC,
  CCSK_JSON_INLINE,
  CCSK_JSON_FILE,
};

struct contract_config_source {
  contract_config_source_kind kind;
  const char *arg;
};

extern vec<contract_config_source> contracts_config_sources;

#endif /* ! GCC_C_FAMILY_CONTRACTS_CONFIG_SOURCE_H */
