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

#include <stdio.h>
#include <string.h>

#include "main.h"
#include "elf.h"

int (* sceKernelCheckExecFile)(u32 *buf, int *check);
int (* CheckElfSectionPRX)(void *buf, u32 *check);
int (* CheckElfSection)(void *buf, u32 *check);
int (* PartitionCheck)(u32 *st0, u32 *check);

int IsStaticElf(void *buf)
{
	Elf32_Ehdr *header = (Elf32_Ehdr *)buf;
	if(header->e_magic == 0x464C457F && header->e_type == 2) return 1;
	return 0;
}

__attribute__((noinline)) char *GetStrTab(u8 *buf)
{
	Elf32_Ehdr *header = (Elf32_Ehdr *)buf;
	if(header->e_magic != 0x464C457F) return NULL;

	u8 *pData = buf + header->e_shoff;

	int i;
	for(i = 0; i < header->e_shnum; i++)
	{
		if(header->e_shstrndx == i)
		{
			Elf32_Shdr *section = (Elf32_Shdr *)pData;
			if(section->sh_type == 3) return (char *)buf + section->sh_offset;
		}
		pData += header->e_shentsize;
	}

	return NULL;
}

int PatchExec2(void *buf, int *check)
{
	int index = check[0x4C/4];
	if(index < 0) index += 3;

	u32 addr = (u32)(buf + index);
	if(addr >= 0x88400000 && addr <= 0x88800000) return 0;

	check[0x58/4] = ((u32 *)buf)[index / 4] & 0xFFFF;
	return ((u32 *)buf)[index / 4];
}

int PatchExec1(void *buf, int *check)
{
	if(((u32 *)buf)[0] != 0x464C457F) return -1;

	if(check[8/4] >= 0x120)
	{
		if(check[0x10/4] == 0)
		{
			if(check[0x44/4] != 0)
			{
				check[0x48/4] = 1;
				return 0;
			}
			return -1;
		}

		check[0x48/4] = 1;
		check[0x44/4] = 1;
		PatchExec2(buf, check);

		return 0;
	}
	else if(check[8/4] >= 0x52) return -1;

	if(check[0x44/4] != 0)
	{
		check[0x48/4] = 1;
		return 0;
	}

	return -2;
}

int PatchExec3(void *buf, int *check, int isPlain, int res)
{
	if(!isPlain) return res;

	if(check[8/4] >= 0x52)
	{
		if(check[0x20/4] == -1) if(IsStaticElf(buf)) check[0x20/4] = 3;
		return res;
	}

	if(!(PatchExec2(buf, check) & 0x0000FF00)) return res;

	check[0x44/4] = 1;
	return 0;
}

int sceKernelCheckExecFilePatched(u32 *buf, int *check)
{
	int res = PatchExec1(buf, check);
	if(res == 0) return res;

	int isPlain = (((u32 *)buf)[0] == 0x464C457F);

	res = sceKernelCheckExecFile(buf, check);

	return PatchExec3(buf, check, isPlain, res);
}

int CheckElfSectionPRXPatched(void *buf, u32 *check)
{
	int res = CheckElfSectionPRX(buf, check);

	if(((u32 *)buf)[0] != 0x464C457F) return res;

	u16 *modinfo = ((u16 *)buf) + (check[0x4C/4] / 2);

	u16 realattr = *modinfo;
	u16 attr = realattr & 0x1E00;

	if(attr != 0)
	{
		u16 attr2 = ((u16 *)check)[0x58/2];
		attr2 &= 0x1E00;

		if(attr2 != attr) ((u16 *)check)[0x58/2] = realattr;
	}

	if(check[0x48/4] == 0) check[0x48/4] = 1;

	return res;
}

int CheckElfSectionPatched(void *buf, u32 *check)
{
	int res = CheckElfSection(buf, check);

	if(((u32 *)buf)[0] != 0x464C457F) return res;

	/* Fake apitype to avoid reject */
	if(IsStaticElf(buf)) check[8/4] = 0x120;

	if(check[0x4C/4] == 0)
	{
		if(IsStaticElf(buf))
		{
			char *strtab = GetStrTab(buf);
			if(strtab)
			{
				Elf32_Ehdr *header = (Elf32_Ehdr *)buf;
				u8 *pData = buf + header->e_shoff;

				int i;
				for(i = 0; i < header->e_shnum; i++)
				{
					Elf32_Shdr *section = (Elf32_Shdr *)pData;

					if(strcmp(strtab + section->sh_name, ".rodata.sceModuleInfo") == 0)
					{
						check[0x4C/4] = section->sh_offset;
						check[0x58/4] = 0;
					}

					pData += header->e_shentsize;
				}
			}
		}
	}

	return res;
}

int PartitionCheckPatched(u32 *st0, u32 *check)
{
	static u32 buf[256/4];

	SceUID fd = (SceUID)st0[0x18/4];
	u16 attributes;

	u32 pos = sceIoLseek(fd, 0, PSP_SEEK_CUR);
	if(pos < 0) return PartitionCheck(st0, check);

	sceIoLseek(fd, 0, PSP_SEEK_SET);
	if(sceIoRead(fd, buf, 256) < 256) //sizeof(buf) doesn't work?
	{
		sceIoLseek(fd, pos, PSP_SEEK_SET);
		return PartitionCheck(st0, check);
	}

	if(buf[0] == 0x50425000)
	{
		sceIoLseek(fd, buf[8], PSP_SEEK_SET);
		sceIoRead(fd, buf, 0x14);

		if(buf[0] != 0x464C457F)
		{
			/* Encrypted module */
			sceIoLseek(fd, pos, PSP_SEEK_SET);
			return PartitionCheck(st0, check);
		}

		sceIoLseek(fd, buf[8] + check[0x4C/4], PSP_SEEK_SET);

		if(!IsStaticElf(buf)) check[0x10/4] = buf[9] - buf[8]; // Allow psar's in decrypted pbp's
	}
	else if(buf[0] == 0x464C457F)
	{
		sceIoLseek(fd, check[0x4C/4], PSP_SEEK_SET);
	}
	else /* encrypted module */
	{
		sceIoLseek(fd, pos, PSP_SEEK_SET);
		return PartitionCheck(st0, check);
	}

	sceIoRead(fd, &attributes, sizeof(attributes));

	if(IsStaticElf(buf)) check[0x44/4] = 0;
	else
	{
		if(attributes & 0x1000) check[0x44/4] = 1;
		else check[0x44/4] = 0;
	}

	sceIoLseek(fd, pos, PSP_SEEK_SET);
	return PartitionCheck(st0, check);
}