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

#include <pspsdk.h>
#include <pspkernel.h>
#include <pspthreadman_kernel.h>
#include <psputilsforkernel.h>
#include <pspsysmem_kernel.h>

#include <stdio.h>
#include <string.h>

#include "main.h"

int kuKernelGetModel()
{
	int k1 = pspSdkSetK1(0);
	int res = sceKernelGetModel660();
	pspSdkSetK1(k1);
	return res;
}

int kuKernelSetDdrMemoryProtection(void *addr, int size, int prot)
{
	int k1 = pspSdkSetK1(0);
	int res = sceKernelSetDdrMemoryProtection660(addr, size, prot);
	pspSdkSetK1(k1);
	return res;
}

int kuKernelGetUserLevel(void)
{
	int k1 = pspSdkSetK1(0);
	int res = sceKernelGetUserLevel();
	pspSdkSetK1(k1);
	return res;
}

int kuKernelInitKeyConfig()
{
	return sceKernelInitKeyConfig();
}

int kuKernelBootFrom()
{
	return sceKernelBootFrom();
}

int kuKernelInitFileName(char *filename)
{
	int k1 = pspSdkSetK1(0);
	strcpy(filename, sceKernelInitFileName());
	pspSdkSetK1(k1);
	return 0;
}

int kuKernelInitApitype()
{
	return sceKernelInitApitype();
}

SceUID kuKernelLoadModuleWithApitype2(int apitype, const char *path, int flags, SceKernelLMOption *option)
{
 	int k1 = pspSdkSetK1(0);
	int res = sceKernelLoadModuleWithApitype2660(apitype, path, flags, option);
	pspSdkSetK1(k1);
	return res;
}

SceUID kuKernelLoadModule(const char *path, int flags, SceKernelLMOption *option)
{
	int k1 = pspSdkSetK1(0);
	int res = sceKernelLoadModule660(path, flags, option);
	pspSdkSetK1(k1);
	return res;
}