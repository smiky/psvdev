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
#include <psputilsforkernel.h>
#include <systemctrl.h>

#include <stdio.h>
#include <string.h>

#include "custom_png.h"

PSP_MODULE_INFO("TNPopcornManager", 0x1007, 1, 0);

#define MAKE_JUMP(a, f) _sw(0x08000000 | (((u32)(f) & 0x0FFFFFFC) >> 2), a);
#define MAKE_CALL(a, f) _sw(0x0C000000 | (((u32)(f) >> 2) & 0x03FFFFFF), a);
#define MAKE_DUMMY_FUNCTION0(a) _sw(0x03E00008, a); _sw(0x00001021, a + 4);
#define MAKE_SYSCALL(a, n) _sw(0x0000000C | (n << 6), a);

typedef struct
{
	u32 header;
	u32 version;
	u32 param_offset;
	u32 icon0_offset;
	u32 icon1_offset;
	u32 pic0_offset;
	u32 pic1_offset;
	u32 snd0_offset;
	u32 elf_offset;
	u32 psar_offset;
} PBPHeader;

char file[64];

int is_custom_psx = 0;
int use_custom_png = 1;

STMOD_HANDLER previous;

int (* SetKeys)(char *filename, void *keys, int unk);
int (* scePopsSetKeys)(int size, void *keys, void *a2);

int (* scePopsManExitVSHKernel)(u32 error);

#define DEBUG

#ifdef DEBUG
#define log(...) { char msg[256]; sprintf(msg,__VA_ARGS__); logmsg(msg); }
#else
#define log(...);
#endif

void logmsg(char *msg)
{
	SceUID fd = sceIoOpen("ms0:/log.txt", PSP_O_WRONLY | PSP_O_CREAT, 0777);
	if(fd > 0)
	{
		sceIoLseek(fd, 0, PSP_SEEK_END);
		sceIoWrite(fd, msg, strlen(msg));
		sceIoClose(fd);
	}
}

void ClearCaches()
{
	sceKernelDcacheWritebackAll();
	sceKernelIcacheClearAll();
}

int ReadFile(char *file, void *buf, int size)
{
	SceUID fd = sceIoOpen(file, PSP_O_RDONLY, 0);
	if(fd < 0) return fd;
	int read = sceIoRead(fd, buf, size);
	sceIoClose(fd);
	return read;
}

int WriteFile(char *file, void *buf, int size)
{
	SceUID fd = sceIoOpen(file, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
	if(fd < 0) return fd;
	int written = sceIoWrite(fd, buf, size);
	sceIoClose(fd);
	return written;
}

SceUID sceIoOpenPatched(const char *file, int flags, SceMode mode)
{
	return sceIoOpen(file, (flags & 0x40000000) ? (flags & ~0x40000000) : flags, mode);
}

int sceIoIoctlPatched(SceUID fd, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen)
{
	if(cmd == 0x04100001 || cmd == 0x04100002)
	{
		if(inlen == 4) sceIoLseek(fd, *(u32 *)indata, PSP_SEEK_SET);
		return 0;
	}

	return sceIoIoctl(fd, cmd, indata, inlen, outdata, outlen);
}

int sceIoReadPatched(SceUID fd, void *data, SceSize size)
{
	int res = sceIoRead(fd, data, size);
	if(res == size)
	{
		if(size == size_custom_png)
		{
			if(use_custom_png)
			{
				u32 magic = 0x474E5089;
				if(memcmp(data, &magic, sizeof(u32)) == 0)
				{
					memcpy(data, custom_png, size_custom_png);
					return size_custom_png;
				}
			}
		}
		else if(size == 4)
		{
			u32 magic = 0x464C457F;
			if(memcmp(data, &magic, sizeof(u32)) == 0)
			{
				magic = 0x5053507E;
				memcpy(data, &magic, sizeof(u32));
				return res;
			}
		}

		/* Unknown patch */
		if(size >= 0x420)
		{
			if(((u8 *)data)[0x41B] == 0x27 && ((u8 *)data)[0x41C] == 0x19)
			{
				if(((u8 *)data)[0x41D] == 0x22 && ((u8 *)data)[0x41E] == 0x41)
				{
					if(((u8 *)data)[0x41A] == ((u8 *)data)[0x41F])
					{
						((u8 *)data)[0x41B] = 0x55;
					}
				}
			}
		}
	}

	return res;
}

int SetKeysPatched(char *filename, void *keys, int unk)
{
	char path[64];
	strcpy(path, filename);

	char *p = strrchr(path, '/');
	if(!p) return 0xCA000000;

	strcpy(p + 1, "KEYS.BIN");

	/* Load keys */
	if(ReadFile(path, keys, 0x10) != 0x10)
	{
		if(is_custom_psx)
		{
			/* Fake keys */
			memset(keys, 'X', 0x10);
		}
		else
		{
			/* Save keys */
			int res = SetKeys(filename, keys, unk);
			if(res >= 0) WriteFile(path, keys, 0x10);
			return res;
		}
	}

	scePopsSetKeys(0x10, keys, keys);
	return 0;
}

int scePopsManExitVSHKernelPatched(u32 destSize, u8 *src, u8 *dest)
{
	int k1 = pspSdkSetK1(0);

	if(destSize < 0)
	{
		scePopsManExitVSHKernel(destSize);
		pspSdkSetK1(k1);
		return 0;
	}

	int size = sceKernelDeflateDecompress(dest, destSize, src, 0);
	pspSdkSetK1(k1);

	return (size ^ 0x9300 ? size : 0x92FF);
}

SceUID sceKernelLoadModulePatched(const char *path, int flags, SceKernelLMOption *option)
{
	return sceKernelLoadModule(file, flags, option);
}

int OnModuleStart(SceModule2 *mod)
{
	char *modname = mod->modname;
	u32 text_addr = mod->text_addr;

	if(strcmp(modname, "pops") == 0)
	{
		_sw(_lw(text_addr + 0x000161A0), text_addr + 0x0000DB78);

		/* Unknown patch */
		_sw(0, text_addr + 0x00025254);

		/* Patch PNG size */
		if(use_custom_png)
		{
			_sw(0x24050000 | (size_custom_png & 0xFFFF), text_addr + 0x00036D50);
		}

		ClearCaches();
	}

	if(!previous)
		return 0;

	return previous(mod);
}

int module_start(SceSize args, void *argp)
{
	/* The pops file */
	strcpy(file, argp);

	char *p = strrchr(file, '/');
	if(!p) return 1;

	strcpy(p + 1, "POPS_01G.PRX");

	/* Open PSX file */
	SceUID fd = sceIoOpen(sceKernelInitFileName(), PSP_O_RDONLY, 0);
	if(fd < 0) return 1;

	SceModule2 *mod = sceKernelFindModuleByName("scePops_Manager");
	u32 text_addr = mod->text_addr;

	PBPHeader header;
	sceIoRead(fd, &header, sizeof(PBPHeader));

	u32 pgd_offset = header.psar_offset;
	u32 icon0_offset = header.icon0_offset;

	u8 buffer[8];
	sceIoLseek(fd, header.psar_offset, PSP_SEEK_SET);
	sceIoRead(fd, buffer, 7);

	if(memcmp(buffer, "PSTITLE", 7) == 0) //official psx game
	{
		pgd_offset += 0x200;
	}
	else
	{
		pgd_offset += 0x400;
	}

	u32 pgd_header;
	sceIoLseek(fd, pgd_offset, PSP_SEEK_SET);
	sceIoRead(fd, &pgd_header, sizeof(u32));

	/* Is not PGD header */
	if(pgd_header != 0x44475000)
	{
		is_custom_psx = 1;

		u32 icon_header[6];
		sceIoLseek(fd, icon0_offset, PSP_SEEK_SET);
		sceIoRead(fd, icon_header, sizeof(icon_header));

		/* Check 80x80 PNG */
		if(icon_header[0] == 0x474E5089 &&
		   icon_header[1] == 0x0A1A0A0D &&
		   icon_header[3] == 0x52444849 &&
		   icon_header[4] == 0x50000000 &&
		   icon_header[5] == 0x50000000)
		{
			use_custom_png = 0;
		}

		previous = sctrlHENSetStartModuleHandler(OnModuleStart);

		int (* sceKernelSetCompiledSdkVersion390)(int sdkversion) = (void *)FindProc("sceSystemMemoryManager", "SysMemUserForUser", 0x315AD3A0);
		if(sceKernelSetCompiledSdkVersion390) sceKernelSetCompiledSdkVersion390(sceKernelDevkitVersion());

		scePopsManExitVSHKernel = (void *)text_addr + 0x00001F30;
		sctrlHENPatchSyscall((u32)scePopsManExitVSHKernel, scePopsManExitVSHKernelPatched);

		/* Patch IO */
		MAKE_JUMP(text_addr + 0x000034F4, sceIoOpenPatched);
		MAKE_JUMP(text_addr + 0x00003504, sceIoIoctlPatched);
		MAKE_JUMP(text_addr + 0x0000350C, sceIoReadPatched);

		/* ... */
		_sw(0, text_addr + 0x0000053C);

		/* Bypass PGD magic check */
		_sw(0, text_addr + 0x0000121C);

		/* Dummy decryption functions */
		MAKE_DUMMY_FUNCTION0(text_addr + 0x00000A00);
		MAKE_DUMMY_FUNCTION0(text_addr + 0x00000A90);
		MAKE_DUMMY_FUNCTION0(text_addr + 0x00000E84);
	}

	sceIoClose(fd);

	scePopsSetKeys = (void *)text_addr + 0x00000124;

	SetKeys = (void *)text_addr + 0x00002638;
	MAKE_CALL(text_addr + 0x00000C64, SetKeysPatched);
	MAKE_CALL(text_addr + 0x00002EE0, SetKeysPatched);

	/* From neur0n's popcorn. Unknown patch */
	_sw(0x00002021, text_addr + 0x00002D98);
	_sw(0x00001021, text_addr + 0x00002D9C);

	/* Avoid standby bug */
	//_sw(0, text_addr + 0x000002B4);

	/* Remove breakpoint */
	_sw(0, text_addr + 0x00001E58);

	MAKE_CALL(text_addr + 0x00001EB8, sceKernelLoadModulePatched);

	ClearCaches();

	return 0;
}