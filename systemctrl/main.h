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

#ifndef __MAIN_H__
#define __MAIN_H__

#include <psploadexec_kernel.h>
#include <pspctrl.h>
#include <systemctrl.h>

#define FW_TO_FIRMWARE(f) ((((f >> 8) & 0xF) << 24) | (((f >> 4) & 0xF) << 16) | ((f & 0xF) << 8) | 0x10)
#define FIRMWARE_TO_FW(f) ((((f >> 24) & 0xF) << 8) | (((f >> 16) & 0xF) << 4) | ((f >> 8) & 0xF))

#define MAKE_JUMP(a, f) _sw(0x08000000 | (((u32)(f) & 0x0FFFFFFC) >> 2), a);
#define MAKE_CALL(a, f) _sw(0x0C000000 | (((u32)(f) >> 2) & 0x03FFFFFF), a);
#define MAKE_SYSCALL(a, n) _sw(0x0000000C | (n << 6), a);

#define REDIRECT_FUNCTION(a, f) \
{ \
	_sw(0x08000000 | (((u32)(f) >> 2) & 0x03FFFFFF), a); \
	_sw(0, a + 4); \
}

#define MAKE_DUMMY_FUNCTION0(a) \
{ \
	_sw(0x03E00008, a); \
	_sw(0x00001021, a + 4); \
}

#define MAKE_DUMMY_FUNCTION1(a) \
{ \
	_sw(0x03E00008, a); \
	_sw(0x24020001, a + 4); \
}

//by Bubbletune
#define U_EXTRACT_IMPORT(x) ((((u32)_lw((u32)x)) & ~0x08000000) << 2)
#define K_EXTRACT_IMPORT(x) (((((u32)_lw((u32)x)) & ~0x08000000) << 2) | 0x80000000)
#define U_EXTRACT_CALL(x) ((((u32)_lw((u32)x)) & ~0x0C000000) << 2)
#define K_EXTRACT_CALL(x) (((((u32)_lw((u32)x)) & ~0x0C000000) << 2) | 0x80000000)

//by Davee
#define HIJACK_FUNCTION(a, f, ptr) \
{ \
	static u32 patch_buffer[3]; \
	_sw(_lw(a + 0x00), (u32)patch_buffer + 0x00); \
	_sw(_lw(a + 0x04), (u32)patch_buffer + 0x08);\
	MAKE_JUMP((u32)patch_buffer + 0x04, a + 0x08); \
	REDIRECT_FUNCTION(a, f); \
	ptr = (void *)patch_buffer; \
}

//#define DEBUG

#ifdef DEBUG

#define log(...) \
{ \
	char msg[256]; \
	sprintf(msg,__VA_ARGS__); \
	logmsg(msg); \
}

#else

#define log(...);

#endif

void logmsg(char *msg);

struct PspModuleImport
{
	const char *name;
	unsigned short version;
	unsigned short attribute;
	unsigned char entLen;
	unsigned char varCount;
	unsigned short funcCount;
	u32 *fnids;
	u32 *funcs;
	u32 *vnids;
	u32 *vars;
} __attribute__((packed));

extern int setjmp_clone();
extern void longjmp_clone();

SceModule2 *sceKernelFindModuleByName660(const char *modname);
SceModule2 *sceKernelFindModuleByAddress660(u32 addr);
SceModule2 *sceKernelFindModuleByUID660(SceUID modid);

SceUID sceKernelLoadModule660(const char *path, int flags, SceKernelLMOption *option);
SceUID sceKernelLoadModuleWithApitype2660(int apitype, const char *path, int flags, SceKernelLMOption *option);
int sceKernelStartModule660(SceUID modid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option);

int sceKernelExitVSHVSH660(struct SceKernelLoadExecVSHParam *param);
int sceKernelCheckExitCallback660();
int sceKernelInvokeExitCallback660();

SceUID sceKernelCreateHeap660(SceUID partitionid, SceSize size, int unk, const char *name);
void *sceKernelAllocHeapMemory660(SceUID heapid, SceSize size);
int sceKernelFreeHeapMemory660(SceUID heapid, void *block);
int sceKernelSetDdrMemoryProtection660(void *addr, int size, int prot);
int sceKernelGetModel660();

int sceCtrlPeekBufferPositive660(SceCtrlData *pad_data, int count);

void ClearCaches();

#endif
