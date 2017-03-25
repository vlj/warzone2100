/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/*! \file
 *  \brief Simple type definitions.
 */

#ifndef __INCLUDED_LIB_FRAMEWORK_TYPES_H__
#define __INCLUDED_LIB_FRAMEWORK_TYPES_H__

#include "wzglobal.h"
#include <cstdint>

#ifdef WZ_CC_MSVC
# define PRIu32					"u"
# define PRIu64					"I64u"
# define PRId64					"I64d"
typedef SSIZE_T ssize_t;
#endif

#ifndef INT8_MAX
#error inttypes.h and stdint.h defines missing! Make sure that __STDC_FORMAT_MACROS and __STDC_LIMIT_MACROS are defined when compiling C++ files.
#endif

#include <limits.h>
#include <ctype.h>

/* Numeric size defines */
#define uint8_t_MAX	UINT8_MAX
#define int8_t_MIN	INT8_MIN
#define int8_t_MAX	INT8_MAX
#define uint16_t_MAX	UINT16_MAX
#define int16_t_MIN	INT16_MIN
#define int16_t_MAX	INT16_MAX
#define uint32_t_MAX	UINT32_MAX
#define int32_t_MIN	INT32_MIN
#define int32_t_MAX	INT32_MAX

#endif // __INCLUDED_LIB_FRAMEWORK_TYPES_H__
