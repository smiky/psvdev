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

#ifndef __LIBC_H__
#define __LIBC_H__

#include <pspkernel.h>

char *strcpy(char *dst, const char *src);
u32 strlen(const char *s);
char *strcat(char *s, const char *append);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t count);
void *memcpy(void *dst, const void *src, int len);
void *memset(void *b, int c, size_t len);

#endif
