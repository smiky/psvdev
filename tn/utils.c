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

#include "main.h"
#include "libc.h"

int ValidUserAddress(void *addr)
{
	if((u32)addr >= 0x08800000 && (u32)addr < 0x0A000000) return 1;
	return 0;
}

u32 FindImport(char *libname, u32 nid)
{
	u32 i;
	for(i = 0x08800000; i < 0x0A000000; i += 4)
	{
		SceLibraryStubTable *stub = (SceLibraryStubTable *)i;

		if((stub->libname != libname) && ValidUserAddress((void *)stub->libname) && ValidUserAddress(stub->nidtable) && ValidUserAddress(stub->stubtable))
		{
			if(strcmp(libname, stub->libname) == 0)
			{
				u32 *nids = stub->nidtable;

				int count;
				for(count = 0; count < stub->stubcount; count++)
				{
					if(nids[count] == nid)
					{
						return ((u32)stub->stubtable + (count * 8));
					}
				}
			}
		}
	}

	return 0;
}

u32 FindFunction(const char *szMod, const char *szLib, u32 nid)
{
	struct SceLibraryEntryTable *entry;

	SceModule2 *pMod = _sceKernelFindModuleByName(szMod);
	if(!pMod) return 0;

	void *entTab = pMod->ent_top;
	int entLen = pMod->ent_size;

	int i = 0;
	while(i < entLen)
	{
		entry = (struct SceLibraryEntryTable *)(entTab + i);

		if(entry->libname && strcmp(entry->libname, szLib) == 0)
		{
			int total = entry->stubcount + entry->vstubcount;
			u32 *table = entry->entrytable;

			if(entry->stubcount > 0)
			{
				int count;
				for(count = 0; count < entry->stubcount; count++)
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