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
#include <pspctrl.h>
#include <pspkernel.h>
#include <psputilsforkernel.h>

#include <stdio.h>
#include <string.h>

#include "nid_table.h"

#include "main.h"
#include "elf.h"
#include "iopatch.h"
#include "conf.h"
#include "cpu.h"
#include "plugins.h"
#include "sysmodpatches.h"
#include "malloc.h"
#include "string_clone.h"

#include "../rebootex/rebootex.h"

PSP_MODULE_INFO("SystemControl", 0x3007, 2, 5);

int (* GetFunction)(void *lib, u32 nid, int unk, int unk2);

void (* sceKernelDcacheWBinvAll660)();
void (* sceKernelIcacheClearAll660)();

int (* DecryptPRX)(u32 *buf, int size, int *retSize, int m);
int (* DecryptPBP)(u32 *tag, u8 *keys, u32 code, u32 *buf, int size, int *retSize, int m, void *unk0, int unk1, int unk2, int unk3, int unk4);

int (* PrologueModule)(void *modmgr_param, SceModule2 *mod);

STMOD_HANDLER module_handler;

SceUID param_fd = 0;

#ifdef DEBUG

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

#endif

void ClearCaches()
{
	sceKernelIcacheInvalidateAll();
	sceKernelDcacheWritebackInvalidateAll();
}

int sceIoIoctlPatched(SceUID fd, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen)
{
	int res = sceIoIoctl(fd, cmd, indata, inlen, outdata, outlen);

	if(res < 0)
	{
		if(param_fd != fd) //Fixes Tekken and Soul Calibur
		{
			if(cmd & 0x00208000)
			{
				res = 0;
			}
		}
	}

	return res;
}

SceUID sceIoOpenPatched(const char *file, int flags, SceMode mode)
{
	SceUID fd = sceIoOpen(file, flags, mode);

	param_fd = 0;
	if(strstr(file, "PARAM.SFO")) param_fd = fd;

	return fd;
}

int PrologueModulePatched(void *modmgr_param, SceModule2 *mod)
{
	int res = PrologueModule(modmgr_param, mod);
	if(res >= 0 && module_handler) module_handler(mod);
	return res;
}

STMOD_HANDLER sctrlHENSetStartModuleHandler(STMOD_HANDLER handler)
{
	STMOD_HANDLER prev = module_handler;
	module_handler = (STMOD_HANDLER)((u32)handler | 0x80000000);
	return prev;
}

void sctrlHENPatchSyscall(u32 addr, void *newaddr)
{
	void *ptr;
	asm("cfc0 %0, $12\n" : "=r"(ptr));

	u32 *syscalls = (u32 *)(ptr + 0x10);

	int i;
	for(i = 0; i < 0x1000; i++)
	{
		if((syscalls[i] & 0x0FFFFFFF) == (addr & 0x0FFFFFFF))
		{
			syscalls[i] = (u32)newaddr;
		}
	}
}

int UnpackCustomModule(u32 *buf, int *retSize)
{
	if(*(u16 *)((u32)buf + 0x150) == 0x8B1F)
	{
		*retSize = *(u32 *)((u32)buf + 0xB0);
		memcpy(buf, (void *)((u32)buf + 0x150), *retSize);
		return 0;
	}

	return -1;
}

int DecryptPBPPatched(u32 *tag, u8 *keys, u32 code, u32 *buf, int size, int *retSize, int m, void *unk0, int unk1, int unk2, int unk3, int unk4)
{
	if(tag && buf && retSize)
	{
		int res = UnpackCustomModule(buf, retSize);
		if(res == 0) return res;
	}

	return DecryptPBP(tag, keys, code, buf, size, retSize, m, unk0, unk1, unk2, unk3, unk4);
}

int DecryptPRXPatched(u32 *buf, int size, int *retSize, int m)
{
	if(buf && retSize)
	{
		int res = UnpackCustomModule(buf, retSize);
		if(res == 0) return res;
	}

	return DecryptPRX(buf, size, retSize, m);
}

__attribute__((noinline)) u32 GetNewNID(NidTable *nid_table, u32 nid)
{
	int i;
	for(i = 0; i < nid_table->n_nid; i++)
	{
		if(nid_table->nid[i].old_nid == nid)
		{
			return nid_table->nid[i].new_nid;
		}
	}

	return 0;
}

NidTable *GetLibraryByName(const char *libname)
{
	if(libname)
	{
		int i = 0;
		do
		{
			if(strcmp(libname, nid_table_660[i].libname) == 0) return nid_table_660 + i;
			i++;
		}
		while(i < NIDS_N);
	}
	return 0;
}

u32 sctrlHENFindFunction(const char *szMod, const char *szLib, u32 nid)
{
	SceModule2 *pMod = sceKernelFindModuleByName660(szMod);
	if(!pMod)
	{
		pMod = sceKernelFindModuleByAddress660((SceUID)szMod);
		if(!pMod) return 0;
	}

	NidTable *nid_table = GetLibraryByName(szLib);
	if(nid_table)
	{
		u32 new_nid = GetNewNID(nid_table, nid);
		if(new_nid) nid = new_nid;
	}

	void *entTab = pMod->ent_top;
	int entLen = pMod->ent_size;

	int i = 0;
	while(i < entLen)
	{
		struct SceLibraryEntryTable *entry = (struct SceLibraryEntryTable *)(entTab + i);

		if(entry->libname && strcmp(entry->libname, szLib) == 0)
		{
			u32 *table = entry->entrytable;
			int total = entry->stubcount + entry->vstubcount;

			if(total > 0)
			{
				int count;
				for(count = 0; count < total; count++)
				{
					if(table[count] == nid)
					{
						return table[count + total];
					}
				}
			}
		}

		i += (entry->len * 4);
	}

	return 0;
}

u32 FindProc(char* szMod, char* szLib, u32 nid) __attribute__((alias("sctrlHENFindFunction")));

u32 sctrlHENFindImport(char *prxname, char *importlib, u32 nid)
{
	SceModule2 *pMod = sceKernelFindModuleByName660(prxname);
	if(!pMod) return 0;

	NidTable *nid_table = GetLibraryByName(importlib);
	if(nid_table)
	{
		u32 new_nid = GetNewNID(nid_table, nid);
		if(new_nid) nid = new_nid;
	}

	void *stubTab = pMod->stub_top;
	int stubLen = pMod->stub_size;

	int i = 0;
	while(i < stubLen)
	{
		struct PspModuleImport *pImp = (struct PspModuleImport *)(stubTab + i);

		if(pImp->name && strcmp(pImp->name, importlib) == 0)
		{
			if(pImp->funcCount > 0)
			{
				int count;
				for(count = 0; count < pImp->funcCount; count++)
				{
					if(pImp->fnids[count] == nid)
					{
						return (u32)&pImp->funcs[count * 2];
					}
				}
			}
		}

		i += (pImp->entLen * 4);
	}

	return 0;
}

int GetFunctionPatched(void *lib, u32 nid, void *stub, int count)
{
	char *libname = (char *)((u32 *)lib)[0x44/4];
	u32 stubtable = ((u32 *)stub)[0x18/4];
	u32 original_stub = ((u32 *)stub)[0x24/4];
	int is_user_mode = ((u32 *)stub)[0x34/4];
	u32 stub_addr = stubtable + (count * 8);

	/* Kernel Module */
	if(!is_user_mode)
	{
		u32 module_sdk_version = FindProc((char *)original_stub, NULL, 0x11B97506);
		if((!module_sdk_version) || (_lw(module_sdk_version) != FW_TO_FIRMWARE(0x660)))
		{
			/* Resolve missing NIDs */
			if(strcmp(libname, "SysclibForKernel") == 0)
			{
				if(nid == 0x909C228B)
				{
					REDIRECT_FUNCTION(stub_addr, &setjmp_clone);
					return -1;
				}
				else if(nid == 0x18FE80DB)
				{
					REDIRECT_FUNCTION(stub_addr, &longjmp_clone);
					return -1;
				}
				else if(nid == 0x1AB53A58)
				{
					REDIRECT_FUNCTION(stub_addr, strtok_r_clone);
					return -1;
				}
				else if(nid == 0x87F8D2DA)
				{
					REDIRECT_FUNCTION(stub_addr, strtok_clone);
					return -1;
				}
				else if(nid == 0x1D83F344)
				{
					REDIRECT_FUNCTION(stub_addr, atob_clone);
					return -1;
				}
				else if(nid == 0x62AE052F)
				{
					REDIRECT_FUNCTION(stub_addr, strspn_clone);
					return -1;
				}
				else if(nid == 0x89B79CB1)
				{
					REDIRECT_FUNCTION(stub_addr, strcspn_clone);
					return -1;
				}
			}
			else if(strcmp(libname, "LoadCoreForKernel") == 0)
			{
				if(nid == 0x2952F5AC)
				{
					REDIRECT_FUNCTION(stub_addr, sceKernelDcacheWBinvAll660);
					return -1;
				}
				else if(nid == 0xD8779AC6)
				{
					REDIRECT_FUNCTION(stub_addr, sceKernelIcacheClearAll660);
					return -1;
				}
			}

			/* Resolve old NIDs */
			NidTable *nid_table = GetLibraryByName(libname);
			if(nid_table)
			{
				u32 new_nid = GetNewNID(nid_table, nid);
				if(new_nid) nid = new_nid;
			}
		}
	}

	int res = GetFunction(lib, nid, -1, 0);

	/* Not linked yet */
	if(res < 0)
	{
		log("Missing: %s - 0x%08X\n", libname, nid);

		_sw(0x0000054C, stub_addr);
		_sw(0x00000000, stub_addr + 4);

		return -1;
	}

	return res;
}

void PatchMsfsDriver()
{
	MAKE_JUMP(sctrlHENFindImport("sceKermitMsfs_driver", "IoFileMgrForKernel", 0x8E982A74), sceIoAddDrvPatched);
	ClearCaches();
}

void PatchMesgLed(u32 text_addr)
{
	HIJACK_FUNCTION(text_addr + 0xE0, DecryptPBPPatched, DecryptPBP);
	ClearCaches();
}

void PatchMemlmd()
{
	SceModule2 *mod = sceKernelFindModuleByName660("sceMemlmd");
	u32 text_addr = mod->text_addr;

	HIJACK_FUNCTION(text_addr + 0x20C, DecryptPRXPatched, DecryptPRX);
}

void PatchInterruptMgr()
{
	SceModule2 *mod = sceKernelFindModuleByName660("sceInterruptManager");
	u32 text_addr = mod->text_addr;

	/* Allow execution of syscalls in kernel mode */
	_sw(0x408F7000, text_addr + 0xE98);
	_sw(0, text_addr + 0xE9C);

	/* Remove 0xBC000004/0xBC000008 protection */
	_sw(0, text_addr + 0xDEC);
	_sw(0, text_addr + 0xDF0);
}

void PatchModuleMgr()
{
	SceModule2 *mod = sceKernelFindModuleByName660("sceModuleManager");
	u32 text_addr = mod->text_addr;

	int i;
	for(i = 0; i < mod->text_size; i += 4)
	{
		u32 addr = text_addr + i;
		if(_lw(addr) == 0xA4A60024)
		{
			/* Patch to allow a full coverage of loaded modules */
			PrologueModule = (void *)K_EXTRACT_CALL(addr - 4);
			MAKE_CALL(addr - 4, PrologueModulePatched);
		}
		else if(_lw(addr) == 0x27BDFFE0 && _lw(addr + 4) == 0xAFB10014)
		{
			HIJACK_FUNCTION(addr, PartitionCheckPatched, PartitionCheck);
		}
	}

	/* No device patch */
	MAKE_JUMP(sctrlHENFindImport("sceModuleManager", "IoFileMgrForKernel", 0x109F50BC), sceIoOpenPatched);
	MAKE_JUMP(sctrlHENFindImport("sceModuleManager", "IoFileMgrForKernel", 0x63632449), sceIoIoctlPatched);
}

void PatchLoadCore()
{
	SceModule2 *mod = sceKernelFindModuleByName660("sceLoaderCore");
	u32 text_addr = mod->text_addr;

	HIJACK_FUNCTION(text_addr + 0x3F20, sceKernelCheckExecFilePatched, sceKernelCheckExecFile);

	/* Patch relocation check in switch statement */
	_sw(_lw(text_addr + 0x7DBC), text_addr + 0x7DD8);

	/* Patch functions called by sceKernelProbeExecutableObject */
	MAKE_CALL(text_addr + 0x4424, CheckElfSectionPRXPatched);
	MAKE_CALL(text_addr + 0x4624, CheckElfSectionPatched);
	MAKE_CALL(text_addr + 0x6520, CheckElfSectionPatched);

	/* Allow kernel modules to have syscall imports */
	_sw(0x3C090000, text_addr + 0x3CE4);

	/* Allow higher devkit version */
	_sh(0x1000, text_addr + 0x69F2);
	_sw(0, text_addr + 0x71F0);

	/* Patch to resolve NIDs */
	_sw(0x02203021, text_addr + 0x39B4); //move $a2, $s1
	MAKE_CALL(text_addr + 0x39B8, GetFunctionPatched);
	_sw(0x02403821, text_addr + 0x39BC); //move $a3, $s2
	_sw(0, text_addr + 0x3B20);
	_sw(0, text_addr + 0x3B28);

	/* Patch call to init module_bootstart */
	MAKE_CALL(text_addr + 0x199C, PatchInit);
	_sw(0x02E02021, text_addr + 0x19A0); //move $a0, $s7

	/* Restore original calls */
	MAKE_CALL(text_addr + 0x3DE4, text_addr + 0x766C);
	MAKE_CALL(text_addr + 0x58AC, text_addr + 0x766C);

	CheckElfSectionPRX = (void *)text_addr + 0x62B4;
	CheckElfSection = (void *)text_addr + 0x620C;
	GetFunction = (void *)text_addr + 0xEA8;

	sceKernelDcacheWBinvAll660 = (void *)text_addr + 0x7298;
	sceKernelIcacheClearAll660 = (void *)text_addr + 0x72D8;
}

__attribute__((noinline)) void PatchSysmem()
{
	static u32 compiled_sdk_check_list[] = { 0x98F0, 0x9A10, 0x9AA8, 0x9B58, 0x9C2C, 0x9CD0, 0x9D74, 0x9E0C, 0x9EBC, 0x9F6C };

	int i;
	for(i = 0; i < (sizeof(compiled_sdk_check_list) / sizeof(u32)); i++)
	{
		_sh(0x1000, 0x88000000 + compiled_sdk_check_list[i] + 2);
	}
}

void OnModuleStart(SceModule2 *mod)
{
	char *modname = mod->modname;
	u32 text_addr = mod->text_addr;

	if(strcmp(modname, "sceKermitMsfs_driver") == 0)
	{
		PatchMsfsDriver();
	}
	if(strcmp(modname, "scePower_Service") == 0)
	{
		log("\n\n");
		log("sceKernelInitFileName: %s\n", sceKernelInitFileName());
		log("sceKernelBootFrom: 0x%08X\n", sceKernelBootFrom());
		log("sceKernelInitApitype: 0x%08X\n", sceKernelInitApitype());
		log("Umdfilename: %s\n", rebootex_config.umdfilename);
		log("Bootfileindex: 0x%08X\n", rebootex_config.bootfileindex);

		sctrlSEGetConfig(&conf);
		PatchPowerService(mod);
	}
	else if(strcmp(modname, "sceLoadExec") == 0)
	{
		PatchLoadExec(text_addr);
	}
	else if(strcmp(modname, "sceMesgLed") == 0)
	{
		PatchMesgLed(text_addr);
	}
	else if(strcmp(modname, "sceMediaSync") == 0)
	{
		PatchMediaSync(text_addr);
	}

	log("Modname: %s\n", modname);
}

int module_start()
{
	PatchSysmem();
	PatchLoadCore();
	PatchModuleMgr();
	PatchMemlmd();
	PatchInterruptMgr();
	ClearCaches();

	module_handler = (STMOD_HANDLER)((u32)OnModuleStart | 0x80000000);

	memcpy(&rebootex_config, (void *)0x88FB0000, sizeof(RebootexConfig));

	mallocinit();

	ClearCaches();

	return 0;
}
