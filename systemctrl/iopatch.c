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
#include "conf.h"

int (* msIoOpen)(PspIoDrvFileArg *arg, char *file, int flags, SceMode mode);
int (* msIoRemove)(PspIoDrvFileArg *arg, const char *name);
int (* msIoDopen)(PspIoDrvFileArg *arg, const char *dirname); 
int (* msIoDread)(PspIoDrvFileArg *arg, SceIoDirent *dir);
int (* msIoDclose)(PspIoDrvFileArg *arg);
int (* msIoGetstat)(PspIoDrvFileArg *arg, const char *file, SceIoStat *stat);
int (* msIoChstat)(PspIoDrvFileArg *arg, const char *file, SceIoStat *stat, int bits);
int (* msIoRename)(PspIoDrvFileArg *arg, const char *oldname, const char *newname);

#define MAX_FILES 32

typedef struct
{
	PspIoDrvFileArg *arg;
	u8 added;
} FileHandler;

FileHandler handlers[MAX_FILES];

int isLicenceFile(const char *file)
{
	if(strncmp(file, "/PSP/LICENSE/", 13) == 0)
	{
		return 1;
	}

	return 0;
}

int isEbootToFix(const char *file)
{
	char *p = strstr(file, "EBOOT.PBP");
	if(p)
	{
		p[0]++;
		return 1;
	}

	return 0;
}

int msIoOpenPatched(PspIoDrvFileArg *arg, char *file, int flags, SceMode mode)
{
	if(isLicenceFile(file) && (mode & PSP_O_WRONLY)) return -1;

	int res = msIoOpen(arg, file, flags, mode);
	if(res < 0 && isEbootToFix(file)) res = msIoOpen(arg, file, flags, mode);
	return res;
}

int msIoRemovePatched(PspIoDrvFileArg *arg, const char *name)
{
	if(isLicenceFile(name)) return -1;

	int res = msIoRemove(arg, name);
	if(res < 0 && isEbootToFix(name)) res = msIoRemove(arg, name);
	return res;
}

int msIoGetstatPatched(PspIoDrvFileArg *arg, const char *file, SceIoStat *stat)
{
	int res = msIoGetstat(arg, file, stat);
	if(res < 0 && isEbootToFix(file)) res = msIoGetstat(arg, file, stat);
	return res;
}

int msIoChstatPatched(PspIoDrvFileArg *arg, const char *file, SceIoStat *stat, int bits)
{
	int res = msIoChstat(arg, file, stat, bits);
	if(res < 0 && isEbootToFix(file)) res = msIoChstat(arg, file, stat, bits);
	return res;
}

int msIoRenamePatched(PspIoDrvFileArg *arg, const char *oldname, const char *newname)
{
	if(isLicenceFile(oldname)) return -1;

	int res = msIoRename(arg, oldname, newname);
	if(res < 0 && (isEbootToFix(oldname) || isEbootToFix(newname))) res = msIoRename(arg, oldname, newname);
	return res;
}

int msIoDopenPatched(PspIoDrvFileArg *arg, const char *dirname)
{
	if(strcmp(dirname, "/") != 0) //not root entry
	{
		if(dirname[strlen(dirname) - 1] == '/') //has got slash at end
		{
			int i;
			for(i = 0; i < MAX_FILES; i++)
			{
				if(handlers[i].arg == 0)
				{
					handlers[i].arg = arg;
					break;
				}
			}
		}
	}

	return msIoDopen(arg, dirname);
}

int msIoDreadPatched(PspIoDrvFileArg *arg, SceIoDirent *dir)
{
	int i;
	for(i = 0; i < MAX_FILES; i++)
	{
		if(handlers[i].arg == arg && handlers[i].added < 2)
		{
			memset(dir, 0, sizeof(SceIoDirent));

			if(handlers[i].added == 0)
			{
				strcpy(dir->d_name, ".");
			}
			else if(handlers[i].added == 1)
			{
				strcpy(dir->d_name, "..");
			}

			dir->d_stat.st_attr |= FIO_SO_IFDIR;
			dir->d_stat.st_mode |= FIO_S_IFDIR;

			handlers[i].added++;

			return handlers[i].added;
		}
	}

	int res = msIoDread(arg, dir);

	if(conf.hide_cfw_folders)
	{
		if(sceKernelBootFrom() == PSP_BOOT_DISC)
		{
			if((strcmp(dir->d_name, "ISO") == 0) || (strcmp(dir->d_name, "iso") == 0) ||
			   (strcmp(dir->d_name, "SEPLUGINS") == 0) || (strcmp(dir->d_name, "seplugins") == 0))
			{
				//skip
				res = msIoDread(arg, dir);
			}
		}
	}

	return res;
}

int msIoDclosePatched(PspIoDrvFileArg *arg)
{
	int i;
	for(i = 0; i < MAX_FILES; i++)
	{
		if(handlers[i].arg == arg)
		{
			handlers[i].arg = 0;
			handlers[i].added = 0;
			break;
		}
	}

	return msIoDclose(arg);
}

int sceIoAddDrvPatched(PspIoDrv *drv)
{
	if(drv->name)
	{
		if(strcmp(drv->name, "ms") == 0)
		{
			memset(handlers, 0, sizeof(handlers));

			msIoOpen = drv->funcs->IoOpen;
			msIoRemove = drv->funcs->IoRemove;
			msIoDopen = drv->funcs->IoDopen;
			msIoDread = drv->funcs->IoDread;
			msIoDclose = drv->funcs->IoDclose;
			msIoGetstat = drv->funcs->IoGetstat;
			msIoChstat = drv->funcs->IoChstat;
			msIoRename = drv->funcs->IoRename;

			drv->funcs->IoOpen = msIoOpenPatched;
			drv->funcs->IoRemove = msIoRemovePatched;
			drv->funcs->IoDopen = msIoDopenPatched;
			drv->funcs->IoDclose = msIoDclosePatched;
			drv->funcs->IoDread = msIoDreadPatched;
			drv->funcs->IoGetstat = msIoGetstatPatched;
			drv->funcs->IoChstat = msIoChstatPatched;
			drv->funcs->IoRename = msIoRenamePatched;
		}
	}

	return sceIoAddDrv(drv);
}