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
#include <psploadexec_kernel.h>
#include <psputility_modules.h>

#include "main.h"
#include "utils.h"
#include "libc.h"

#include "../rebootex/rebootex.h"

void (* _sceKernelIcacheInvalidateAll)() = (void *)0x88000E98;
void (* _sceKernelDcacheWritebackInvalidateAll)() = (void *)0x88000744;
SceModule2 *(*_sceKernelFindModuleByName)(const char *name) = (void *)0x88016F00 + 0x6DE4;

__attribute__((noinline)) void FillVram(u32 color)
{
	u32 *vram = (u32 *)0x44000000;
	while(vram < (u32 *)0x44200000) *vram++ = (u32)color;
}

void ErrorFillVram()
{
	FillVram(0x0000FF);
	_sw(0, 0);
}

int MakeFileList()
{
	SceUID (* _sceIoOpen)(const char *file, int flags, SceMode mode) = (void *)FindFunction("sceIOFileManager", "IoFileMgrForKernel", 0x109F50BC);
	int (* _sceIoRead)(SceUID fd, void *data, SceSize size) = (void *)FindFunction("sceIOFileManager", "IoFileMgrForKernel", 0x6A638D83);
	SceOff (* _sceIoLseek)(SceUID fd, SceOff offset, int whence) = (void *)FindFunction("sceIOFileManager", "IoFileMgrForKernel", 0x27EB27B8);
	int (* _sceIoClose)(SceUID fd) = (void *)FindFunction("sceIOFileManager", "IoFileMgrForKernel", 0x810C4BC3);

	/* Open package */
	char path[64];
	strcpy(path, rebootex_config.savedata_path);
	strcat(path, "/FLASH0.TN");
	SceUID fd = _sceIoOpen(path, PSP_O_RDONLY, 0);
	if(fd < 0) return fd;

	/* Get file_n */
	u32 file_n = 0;

	int size = _sceIoLseek(fd, 0, PSP_SEEK_END);
	_sceIoLseek(fd, size - 4, PSP_SEEK_SET);
	_sceIoRead(fd, &file_n, sizeof(file_n));
	_sceIoLseek(fd, 0, PSP_SEEK_SET);

	/* Get file_list size */
	FileList *file_list = (FileList *)0x8B000000;
	while(file_list->buffer != 0) file_list++;

	u32 file_list_size = (u32)file_list - 0x8B000000;

	/* Add new files */
	memcpy((void *)0x8BA00000 + file_n * sizeof(FileList), (void *)0x8B000000, file_list_size);
	u32 pointer = 0x8BA00000 + file_list_size + (file_n + 1) * sizeof(FileList);
	file_list = (FileList *)0x8BA00000;

	u32 path_pointer = 0x8B000000;

	int res = 0;
	do
	{
		TNPackage package;
		res = _sceIoRead(fd, &package, sizeof(TNPackage));

		if(package.magic == 0x4B504E54)
		{
			/* Add path */
			file_list->name = (void *)path_pointer;
			_sceIoRead(fd, file_list->name, package.path_len);
			path_pointer += package.path_len;

			/* Add buffer */
			pointer = (((u32)pointer + 0x40) & ~0x3F);
			file_list->buffer = (void *)pointer;
			_sceIoRead(fd, (void *)file_list->buffer, package.size);
			pointer += package.size;

			/* Add size */
			file_list->size = package.size;

			/* Next entry */
			file_list++;
		}
	} while(res > 0);

	_sceIoClose(fd);

	return res;
}

void kernel_function()
{
	/* Set k1 */
	asm("move $k1, $0\n");

	/* Repair sysmem */
	_sw(0x0200D821, 0x8800F714);
	_sw(0x3C038801, 0x8800F718);
	_sw(0x8C654384, 0x8800F71C);

	/* Patch loadexec */
	SceModule2 *mod = _sceKernelFindModuleByName("sceLoadExec");
	u32 text_addr = mod->text_addr;

	MAKE_JUMP(text_addr + 0x2E30, Reboot_Entry);

	/* Allow LoadExecVSH in whatever user level */
	_sh(0x1000, text_addr + 0x241E);
	_sw(0, text_addr + 0x2460);

	_sceKernelIcacheInvalidateAll();
	_sceKernelDcacheWritebackInvalidateAll();

	MakeFileList();

	/* Load Eboot */
	int (* LoadExecVSH)(int apitype, const char *file, struct SceKernelLoadExecVSHParam *param, int unk2) = (void *)text_addr + 0x23D0;

	char program[64];
	strcpy(program, rebootex_config.savedata_path);
	strcat(program, "/MENU.PBP");

	struct SceKernelLoadExecVSHParam param;

	memset(&param, 0, sizeof(param));
	param.size = sizeof(param);
	param.argp = program;
	param.args = strlen(param.argp) + 1;
	param.key = "game";

	LoadExecVSH(PSP_INIT_APITYPE_MS2, program, &param, 0x10000);
}

void _start(SceSize args, void *argp) __attribute__((section(".text.start")));
void _start(SceSize args, void *argp)
{
	/* Flash screen */
	void (* _sceDisplaySetFrameBuf)() = (void *)FindImport("sceDisplay", 0x289D82FE);
	if(_sceDisplaySetFrameBuf) _sceDisplaySetFrameBuf((void *)0x44000000, 512, 3, 1);

	FillVram(0xFFFFFF);

	/* Get savedata path */
	strcpy(rebootex_config.savedata_path, argp);
	rebootex_config.savedata_path[args-1] = '\0';

	/* Find imports in RAM */
	void (* _sceKernelDcacheWritebackAll)() = (void *)FindImport("UtilsForUser", 0x79D1C3FA);
	void (* _sceKernelLibcTime)(int a0, int (* a1)) = (void *)FindImport("UtilsForUser", 0x27CC57F0);
	if(!_sceKernelDcacheWritebackAll || !_sceKernelLibcTime) ErrorFillVram();

	int (* _sceWlanGetEtherAddr)(u8 *etherAddr) = (void *)FindImport("sceWlanDrv", 0x0C622081);
	if(!_sceWlanGetEtherAddr)
	{
		/* Load required modules */
		int (* _sceUtilityLoadModule)(int id) = (void *)FindImport("sceUtility", 0x2A2B3DE0);
		if(_sceUtilityLoadModule)
		{
			_sceUtilityLoadModule(PSP_MODULE_NET_COMMON);
			_sceUtilityLoadModule(PSP_MODULE_NET_ADHOC);
		}

		/* Try again */
		_sceWlanGetEtherAddr = (void *)FindImport("sceWlanDrv", 0x0C622081);
		if(!_sceWlanGetEtherAddr) ErrorFillVram();
	}

	/* Manipulate sysmem */
	_sceWlanGetEtherAddr((u8 *)0x8800F718);
	_sceWlanGetEtherAddr((u8 *)0x8800F716);
	_sceWlanGetEtherAddr((u8 *)0x8800F714);
	_sceWlanGetEtherAddr((u8 *)0x8800F712);
	_sceKernelDcacheWritebackAll();

	/* Exec kernel function */
	_sceKernelLibcTime(0, (void *)((u32)kernel_function | 0x80000000));

	while(1); //Infinite loop
}