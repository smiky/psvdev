/*
	Custom Emulator Firmware
	Copyright (C) 2012, Total_Noob

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <pspkernel.h>
#include "libc.h"

char *strcpy(char *dst, const char *src)
{
	char *pRet = dst;
	while(*src) *dst++ = *src++;
	*dst = 0;
	return pRet;
}

u32 strlen(const char *s)
{
	int len = 0;
	while(*s)
	{
		s++;
		len++;
	}

	return len;
}

char *strcat(char *s, const char *append)
{
	char *pRet = s;
	while(*s) s++;
	while(*append)*s++ = *append++;
	*s = 0;
	return pRet;
}

int strcmp(const char *s1, const char *s2)
{
	int val = 0;
	const u8 *u1, *u2;

	u1 = (u8 *)s1;
	u2 = (u8 *)s2;

	while(1)
	{
		if(*u1 != *u2)
		{
			val = (int) *u1 - (int) *u2;
			break;
		}

		if((*u1 == 0) && (*u2 == 0))
		{
			break;
		}

		u1++;
		u2++;
	}

	return val;
}

int strncmp(const char *s1, const char *s2, size_t count)
{
	int val = 0;
	const u8 *u1, *u2;

	u1 = (u8 *) s1;
	u2 = (u8 *) s2;

	while(count > 0)
	{
		if(*u1 != *u2)
		{
			val = (int) *u1 - (int) *u2;
			break;
		}

		if((*u1 == 0) && (*u2 == 0))
		{
			break;
		}

		u1++;
		u2++;
		count--;
	}

	return val;
}

void *memcpy(void *dst, const void *src, int len)
{
	void *pRet = dst;
	const char *usrc = (const char *)src;
	char *udst = (char *)dst;

	while(len > 0)
	{
		*udst++ = *usrc++;
		len--;
	}

	return pRet;
}

void *memset(void *b, int c, size_t len)
{
	void *pRet = b;
	u8 *ub = (u8 *)b;

	while(len > 0)
	{
		*ub++ = (u8)c;
		len--;
	}

	return pRet;
}