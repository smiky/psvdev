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
#include <pspsysmem_kernel.h>

#include <stdio.h>
#include <string.h>

#include "main.h"

SceUID heapid = -1;

void oe_free(void *ptr)
{
	sceKernelFreeHeapMemory660(heapid, ptr);
}

void *oe_malloc(SceSize size)
{
	return sceKernelAllocHeapMemory660(heapid, size);
}

int mallocinit()
{
	heapid = sceKernelCreateHeap660(PSP_MEMORY_PARTITION_KERNEL, 45 * 1024, 1, "");
	return heapid;
}