/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007-2017  Warzone 2100 Project

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

#include "string_ext.h"

char *strdup2(const char *s, char *fileName, int line)
{
	char *result;

	(void)debug_MEMCHKOFF();
	result = (char *)malloc(strlen(s) + 1);
	(void)debug_MEMCHKON();

	if (result == (char *)0)
	{
		return (char *)0;
	}
	strcpy(result, s);
	return result;
}

WZ_DECL_PURE size_t strnlen1(const char *string, size_t maxlen)
{
	// Find the first NUL char
	const char *end = (const char *)memchr(string, '\0', maxlen); // Cast required for C++

	if (end != NULL)
	{
		return end - string + 1;
	}
	else
	{
		return maxlen;
	}
}

size_t strlcpy(char *WZ_DECL_RESTRICT dest, const char *WZ_DECL_RESTRICT src, size_t size)
{
#ifdef DEBUG
	ASSERT_OR_RETURN(0, src != NULL, "strlcpy was passed an invalid src parameter.");
	ASSERT_OR_RETURN(0, dest != NULL, "strlcpy was passed an invalid dest parameter.");
#endif

	if (size > 0)
	{
		strncpy(dest, src, size - 1);
		// Guarantee to nul-terminate
		dest[size - 1] = '\0';
	}

	return strlen(src);
}

size_t strlcat(char *WZ_DECL_RESTRICT dest, const char *WZ_DECL_RESTRICT src, size_t size)
{
	size_t len;

#ifdef DEBUG
	ASSERT_OR_RETURN(0, src != NULL, "strlcat was passed an invalid src parameter.");
	ASSERT_OR_RETURN(0, dest != NULL, "strlcat was passed an invalid dest parameter.");
#endif

	len = strlen(src);

	if (size > 0)
	{
		size_t dlen;

		dlen = strnlen1(dest, size);
		len += dlen;

		assert(dlen > 0);

		strlcpy(&dest[dlen - 1], src, size - dlen);
	}

	return len;
}
