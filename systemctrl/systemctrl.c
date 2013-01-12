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
#include <psploadexec_kernel.h>
#include <pspthreadman_kernel.h>

#include <stdio.h>
#include <string.h>

#include "main.h"

int sctrlHENIsSE()
{
	return 1;
}

int sctrlHENIsDevhook()
{
	return 0;
}

int sctrlHENGetVersion()
{
	return 0x00001020;
}

int sctrlSEGetVersion()
{
	return 0x00020005;
}

PspIoDrv *sctrlHENFindDriver(char *drvname)
{
	int k1 = pspSdkSetK1(0);

	SceModule2 *mod = sceKernelFindModuleByName660("sceIOFileManager");
	u32 text_addr = mod->text_addr;

	u32 *(* GetDevice)(char *) = NULL;

	int i;
	for(i = 0; i < mod->text_size; i += 4)
	{
		u32 addr = text_addr + i;
		if(_lw(addr) == 0xA2200000)
		{
			GetDevice = (void *)K_EXTRACT_CALL(addr + 4);
			break;
		}
	}

	if(!GetDevice)
	{
		pspSdkSetK1(k1);
		return 0;
	}

	u32 *u = GetDevice(drvname);
	if(!u)
	{
		pspSdkSetK1(k1);
		return 0;
	}

	pspSdkSetK1(k1);

	return (PspIoDrv *)u[1];
}

int sctrlKernelLoadExecVSHWithApitype(int apitype, const char *file, struct SceKernelLoadExecVSHParam *param)
{
	int k1 = pspSdkSetK1(0);

	SceModule2 *mod = sceKernelFindModuleByName660("sceLoadExec");
	u32 text_addr = mod->text_addr;

	int (* LoadExecVSH)(int apitype, const char *file, struct SceKernelLoadExecVSHParam *param, int unk2) = (void *)text_addr + 0x23D0;

	int res = LoadExecVSH(apitype, file, param, 0x10000);
	pspSdkSetK1(k1);
	return res;
}

int sctrlKernelExitVSH(struct SceKernelLoadExecVSHParam *param)
{
	int k1 = pspSdkSetK1(0);
	int res = sceKernelExitVSHVSH660(param);
	pspSdkSetK1(k1);
	return res;
}

int sctrlKernelLoadExecVSHMs1(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	return sctrlKernelLoadExecVSHWithApitype(PSP_INIT_APITYPE_MS1, file, param);
}

int sctrlKernelLoadExecVSHMs2(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	return sctrlKernelLoadExecVSHWithApitype(PSP_INIT_APITYPE_MS2, file, param);
}

int sctrlKernelLoadExecVSHMs3(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	return sctrlKernelLoadExecVSHWithApitype(PSP_INIT_APITYPE_MS3, file, param);
}

int sctrlKernelLoadExecVSHMs4(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	return sctrlKernelLoadExecVSHWithApitype(PSP_INIT_APITYPE_MS4, file, param);
}

int sctrlKernelLoadExecVSHDisc(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	return sctrlKernelLoadExecVSHWithApitype(PSP_INIT_APITYPE_DISC, file, param);
}

int sctrlKernelLoadExecVSHDiscUpdater(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	return sctrlKernelLoadExecVSHWithApitype(PSP_INIT_APITYPE_DISC_UPDATER, file, param);
}

int sctrlKernelSetUserLevel(int level)
{
	int k1 = pspSdkSetK1(0);
	int res = sceKernelGetUserLevel();

	SceModule2 *mod = sceKernelFindModuleByName660("sceThreadManager");
	u32 text_addr = mod->text_addr;

	u32 *thstruct = (u32 *)_lw(text_addr + 0x19F40);
	thstruct[0x14/4] = (level ^ 8) << 28;

	pspSdkSetK1(k1);
	return res;
}