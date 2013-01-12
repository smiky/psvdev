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
#include <systemctrl.h>

#include <stdio.h>
#include <string.h>

#include "main.h"
#include "conf.h"

#include "../rebootex/rebootex.h"

int (* sceUtilityGetSystemParamString)(int id, char *str, int len);
int (* sceUtilityGetSystemParamInt)(int id, int *value);
int (* sceUtilityLoadModule)(int id);
int (* sceUtilityUnloadModule)(int id);

char *GetUmdFile()
{
	return rebootex_config.umdfilename;
}

__attribute__((noinline)) void SetUmdFile(char *file)
{
	strncpy(rebootex_config.umdfilename, file, 0x47);
}

int sctrlSEMountUmdFromFile(char *file, int noumd, int isofs)
{
	int k1 = pspSdkSetK1(0);

	SetUmdFile(file);
	rebootex_config.bootfileindex = noumd;

	pspSdkSetK1(k1);
	return 0;
}

int sctrlSEGetBootConfBootFileIndex()
{
	return rebootex_config.bootfileindex;
}

void sctrlSESetBootConfFileIndex(int index)
{
	rebootex_config.bootfileindex = index;
}

void sctrlHENLoadModuleOnReboot(char *module_after, void *buf, int size, int flags)
{
	rebootex_config.reboot_module_after = module_after;
	rebootex_config.reboot_buf = buf;
	rebootex_config.reboot_size = size;
	rebootex_config.reboot_flags = flags;
}

int ExitPatched()
{
	int k1 = pspSdkSetK1(0);

	char program[64];
	sprintf(program, "%s/MENU.PBP", rebootex_config.savedata_path);

	struct SceKernelLoadExecVSHParam param;

	memset(&param, 0, sizeof(param));
	param.size = sizeof(param);
	param.argp = program;
	param.args = strlen(param.argp) + 1;
	param.key = "game";

	int res = sctrlKernelLoadExecVSHMs2(program, &param);
	pspSdkSetK1(k1);
	return res;
}

void PatchLoadExec(u32 text_addr)
{
	MAKE_JUMP(text_addr + 0x2E30, Reboot_Entry);

	/* Allow LoadExecVSH in whatever user level */
	_sh(0x1000, text_addr + 0x241E);
	_sw(0, text_addr + 0x2460);

	/* Allow ExitVSHVSH in whatever user level */
	_sh(0x1000, text_addr + 0x16A6);
	_sw(0, text_addr + 0x16D8);

	/* Exit -> load menu */
	REDIRECT_FUNCTION(text_addr + 0x154C, ExitPatched);

	ClearCaches();
}

int sceUtilityGetSystemParamStringPatched(int id, char *str, int len)
{
	int k1 = pspSdkSetK1(0);

	if(id == 1)
	{
		if(strlen(conf.nickname) > 0)
		{
			memcpy(str, conf.nickname, len);
			pspSdkSetK1(k1);
			return 0;
		}
	}

	pspSdkSetK1(k1);

	return sceUtilityGetSystemParamString(id, str, len);
}

int sceUtilityGetSystemParamIntPatched(int id, int *value)
{
	if(id == 9)
	{
		*value = conf.button_assign;
		return 0;
	}

	return sceUtilityGetSystemParamInt(id, value);
}

int sceUtilityLoadModulePatched(int id)
{
	int res = sceUtilityLoadModule(id);
	if(id == 0x500 && res == 0x80020148) res = 0;
	return res;
}

int sceUtilityUnloadModulePatched(int id)
{
	int res = sceUtilityUnloadModule(id);
	if(id == 0x500 && res == 0x80020148) res = 0;
	return res;
}

int HoldButtons(SceCtrlData *pad, u32 buttons, int time)
{
	if((pad->Buttons & buttons) == buttons)
	{
		u32 time_start = sceKernelGetSystemTimeLow();

		while((pad->Buttons & buttons) == buttons)
		{
			sceKernelDelayThread(100 * 1000);
			sceCtrlPeekBufferPositive660(pad, 1);

			if((sceKernelGetSystemTimeLow() - time_start) >= time) return 1;
		}
	}

	return 0;
}

int button_list[] = { 0, PSP_CTRL_SELECT, PSP_CTRL_START, PSP_CTRL_UP, PSP_CTRL_RIGHT, PSP_CTRL_DOWN, PSP_CTRL_LEFT,
					  PSP_CTRL_LTRIGGER, PSP_CTRL_RTRIGGER, PSP_CTRL_TRIANGLE, PSP_CTRL_CIRCLE, PSP_CTRL_CROSS, PSP_CTRL_SQUARE };

int getCtrlButton(int i)
{
	if(i >= 0 && i < (sizeof(button_list) / sizeof(int))) return button_list[i];
	return 0;
}

SceUID ctrl_thread(SceSize args, void *argp)
{
	SceCtrlData pad;

    while(1)
	{
		sceCtrlPeekBufferPositive660(&pad, 1);

		u32 buttons = getCtrlButton(conf.exit_button_1) | getCtrlButton(conf.exit_button_2);
		if(buttons)
		{
			if(HoldButtons(&pad, buttons, conf.exit_hold_duration * 1000 * 1000))
			{
				if(sceKernelCheckExitCallback660() > 0)
				{
					sceKernelInvokeExitCallback660();
				}
				else
				{
					ExitPatched();
				}
			}
		}

		sceKernelDelayThread(100 * 1000);
	}

	return 0;
}

void PatchNp9660()
{
	/* Fake sceUmdCheckMedium in homebrews */
	if(sceKernelInitApitype() == PSP_INIT_APITYPE_MS2)
	{
		int (* sceUmdCheckMedium)() = (void *)FindProc("sceNp9660_driver", "sceUmdUser", 0x46EBB729);
		if(sceUmdCheckMedium) MAKE_DUMMY_FUNCTION0((u32)sceUmdCheckMedium);
	}
}

void PatchUtility()
{
	sceUtilityGetSystemParamString = (void *)FindProc("sceUtility_Driver", "sceUtility", 0x34B78343);
	sceUtilityGetSystemParamInt = (void *)FindProc("sceUtility_Driver", "sceUtility", 0xA5DA2406);
	sceUtilityLoadModule = (void *)FindProc("sceUtility_Driver", "sceUtility", 0x2A2B3DE0);
	sceUtilityUnloadModule = (void *)FindProc("sceUtility_Driver", "sceUtility", 0xE49BFE92);

	sctrlHENPatchSyscall((u32)sceUtilityGetSystemParamString, sceUtilityGetSystemParamStringPatched);
	sctrlHENPatchSyscall((u32)sceUtilityGetSystemParamInt, sceUtilityGetSystemParamIntPatched);
	sctrlHENPatchSyscall((u32)sceUtilityLoadModule, sceUtilityLoadModulePatched);
	sctrlHENPatchSyscall((u32)sceUtilityUnloadModule, sceUtilityUnloadModulePatched);
}

void PatchMediaSync(u32 text_addr)
{
	SceUID thid = sceKernelCreateThread("ctrl_thread", ctrl_thread, 0x10, 0x1000, 0, NULL);
	if(thid >= 0) sceKernelStartThread(thid, 0, NULL);

	/* Dummy MsCheckMedia */
	MAKE_DUMMY_FUNCTION1(text_addr + 0x744);

	PatchNp9660();
	PatchUtility();

	ClearCaches();
}