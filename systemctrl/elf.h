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

#ifndef __ELF_H__
#define __ELF_H__

typedef struct
{
	u32 e_magic;
	u8 e_class;
	u8 e_data;
	u8 e_idver;
	u8 e_pad[9];
	u16 e_type;
	u16 e_machine;
	u32 e_version;
	u32 e_entry;
	u32 e_phoff;
	u32 e_shoff;
	u32 e_flags;
	u16 e_ehsize;
	u16 e_phentsize;
	u16 e_phnum;
	u16 e_shentsize;
	u16 e_shnum;
	u16 e_shstrndx;
} __attribute__((packed)) Elf32_Ehdr;

typedef struct
{
	u32 sh_name;
	u32 sh_type;
	u32 sh_flags;
	u32 sh_addr;
	u32 sh_offset;
	u32 sh_size;
	u32 sh_link;
	u32 sh_info;
	u32 sh_addralign;
	u32 sh_entsize;
} __attribute__((packed)) Elf32_Shdr;

extern int (* sceKernelCheckExecFile)(u32 *buf, int *check);
extern int (* CheckElfSectionPRX)(void *buf, u32 *check);
extern int (* CheckElfSection)(void *buf, u32 *check);
extern int (* PartitionCheck)(u32 *st0, u32 *check);

int sceKernelCheckExecFilePatched(u32 *buf, int *check);
int CheckElfSectionPRXPatched(void *buf, u32 *check);
int CheckElfSectionPatched(void *buf, u32 *check);
int PartitionCheckPatched(u32 *st0, u32 *check);

#endif