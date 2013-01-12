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

#include "../rebootex/rebootex.h"

int plugins_started = 0;

u32 init_addr;

u32 sctrlGetInitTextAddr()
{
	return init_addr;
}

void trim(char *str)
{
	int i;
	for(i = strlen(str) - 1; i >= 0; i--)
	{
		if(str[i] == 0x20 || str[i] == '\t') str[i] = 0;
		else break;
	}
}

int GetPlugin(char *buf, int size, char *str, int *activated)
{
	char ch = 0;
	int n = 0, i = 0;
	char *s = str;

	while(1)
	{
		if(i >= size) break;

		ch = buf[i];

		if(ch < 0x20 && ch != '\t')
		{
			if(n != 0)
			{
				i++;
				break;
			}
		}
		else
		{
			*str++ = ch;
			n++;
		}

		i++;
	}

	trim(s);

	*activated = 0;

	if(i > 0)
	{
		char *p = strpbrk(s, " \t");
		if(p)
		{
			char *q = p + 1;
			while(*q < 0) q++;
			if(strcmp(q, "1") == 0) *activated = 1;
			*p = 0;
		}   
	}

	return i;
}

int LoadStartModule(char *file)
{
	SceUID mod = sceKernelLoadModule660(file, 0, NULL);
	if(mod < 0) return mod;
	return sceKernelStartModule660(mod, strlen(file) + 1, file, NULL, NULL);
}

int sceKernelStartModulePatched(SceUID modid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option)
{
	SceModule2 *mod = sceKernelFindModuleByUID660(modid);
	if(mod)
	{
		if(!plugins_started)
		{
			if(strcmp(mod->modname, "sceMediaSync") == 0)
			{
				plugins_started = 1;

				SceUID fd = -1;

				char path[64];

				int type = sceKernelInitKeyConfig();
				if(type == PSP_INIT_KEYCONFIG_GAME)
				{
					sprintf(path, "%s/GAME.TXT", rebootex_config.savedata_path);
					fd = sceIoOpen(path, PSP_O_RDONLY, 0);
				}
				else if(type == PSP_INIT_KEYCONFIG_POPS)
				{
					sprintf(path, "%s/POPCORN.PRX", rebootex_config.savedata_path);
					LoadStartModule(path);

					sprintf(path, "%s/POPS.TXT", rebootex_config.savedata_path);
					fd = sceIoOpen(path, PSP_O_RDONLY, 0);
				}

				if(fd >= 0)
				{
					SceUID fpl = sceKernelCreateFpl("", PSP_MEMORY_PARTITION_KERNEL, 0, 1024, 1, NULL);
					if(fpl >= 0)
					{
						char *buffer;
						sceKernelAllocateFpl(fpl, (void *)&buffer, NULL);
						int size = sceIoRead(fd, buffer, 1024);
						char *p = buffer;

						int res = 0;
						char plugin[64];

						do
						{
							int activated = 0;
							memset(plugin, 0, sizeof(plugin));

							res = GetPlugin(p, size, plugin, &activated);

							if(res > 0)
							{
								if(activated) LoadStartModule(plugin);
								size -= res;
								p += res;
							}
						} while(res > 0);

						sceIoClose(fd);

						if(buffer)
						{
							sceKernelFreeFpl(fpl, buffer);
							sceKernelDeleteFpl(fpl);
						}
					}
				}
			}
		}
	}

	return sceKernelStartModule660(modid, argsize, argp, status, option);
}

int PatchInit(int (* module_bootstart)(SceSize, void *), void *argp)
{
	init_addr = ((u32)module_bootstart) - 0x1A54;

	MAKE_JUMP(init_addr + 0x1C5C, sceKernelStartModulePatched);

	ClearCaches();

	return module_bootstart(4, argp);
}