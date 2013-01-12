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

#ifndef __REBOOTEX_H__
#define __REBOOTEX_H__

#define MAKE_JUMP(a, f) _sw(0x08000000 | (((u32)(f) & 0x0FFFFFFC) >> 2), a);
#define MAKE_CALL(a, f) _sw(0x0C000000 | (((u32)(f) >> 2) & 0x03FFFFFF), a);

typedef struct
{
	char umdfilename[0x48];
	int bootfileindex;
	char *reboot_module_after;
	char *reboot_buf;
	int reboot_size;
	int reboot_flags;
	char savedata_path[64];
} RebootexConfig;

int Reboot_Entry(void *reboot_param, void *exec_param, int api, int initial_rnd);

extern RebootexConfig rebootex_config;

#endif