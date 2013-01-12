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
#include <string.h>

#include "rebootex.h"

int (* sceReboot)(void *reboot_param, void *exec_param, int api, int initial_rnd) = (void *)0x88600000;
int (* ClearIcache)() = (void *)0x88601E40;
int (* ClearDcache)() = (void *)0x886018AC;

int (* DecryptExecutable)(void *buf, int size, int *retSize);

RebootexConfig rebootex_config;

void RClearCaches()
{
	ClearIcache();
	ClearDcache();
}

int DecryptExecutablePatched(void *buf, int size, int *retSize)
{
	if(*(u16 *)((u32)buf + 0x150) == 0x8B1F)
	{
		*retSize = *(u32 *)((u32)buf + 0xB0);
		memcpy(buf, (void *)((u32)buf + 0x150), *retSize);
		return 0;
	}

	return DecryptExecutable(buf, size, retSize);
}

int RPatchLoadCore(int (* module_bootstart)(SceSize, void *), void *argp)
{
	u32 text_addr = ((u32)module_bootstart) - 0xAF8;

	MAKE_CALL(text_addr + 0x3DE4, DecryptExecutablePatched);
	MAKE_CALL(text_addr + 0x58AC, DecryptExecutablePatched);

	DecryptExecutable = (void *)text_addr + 0x766C;

	RClearCaches();

	return module_bootstart(8, argp);
}

int Reboot_Entry(void *reboot_param, void *exec_param, int api, int initial_rnd)
{
	if(api != 0x110 && api != 0x120)
	{
		rebootex_config.bootfileindex = 0;
		memset(rebootex_config.umdfilename, 0, 0x48);
	}

	/* Redirect pointer */
	_sb(0xA0, 0x886020DC);
	_sb(0xA0, 0x886020E8);

	/* Patch ~PSP header check (enable non ~PSP config file) */
	_sw(0xAFA50000, 0x88602E84);
	_sw(0x20A30000, 0x88602E88);

	/* Patch call to LoadCore module_bootstart */
	_sw(0x00602021, 0x88602498); //move $a0, $v1
	MAKE_JUMP(0x886024A0, RPatchLoadCore);

	/* Rename bootconfig file */
	static char *pspbtcnf = "/pspbtcnf.bin";
	if(rebootex_config.bootfileindex == 1) pspbtcnf[6] = 'd';
	else if(rebootex_config.bootfileindex == 2) pspbtcnf[6] = 'e';

	strcpy((void *)0x88604BBC, pspbtcnf);

	/* Backup for reboot */
	memcpy((void *)0x88FB0000, &rebootex_config, sizeof(RebootexConfig));

	RClearCaches();

	/* Call original function */
	return sceReboot(reboot_param, exec_param, api, initial_rnd);
}