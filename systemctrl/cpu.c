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

#include "main.h"
#include "conf.h"

int (* SetClock)(int pll, int cpu, int bus);

int cpu_list[] = { 0, 20, 75, 100, 133, 166, 222, 266, 300, 333 };
int bus_list[] = { 0, 10, 37,  50,  66,  83, 111, 133, 150, 166 };

int getCpuSpeed(int i)
{
	if(i >= 0 && i < (sizeof(cpu_list) / sizeof(int))) return cpu_list[i];
	return 0;
}

int getBusSpeed(int i)
{
	if(i >= 0 && i < (sizeof(bus_list) / sizeof(int))) return bus_list[i];
	return 0;
}

int SetClockPatched(int pll, int cpu, int bus)
{
	int new_cpu = getCpuSpeed(conf.cpu_speed);
	if(new_cpu > 0)
	{
		pll = cpu = new_cpu;
		bus = getBusSpeed(conf.cpu_speed);
	}

	return SetClock(pll, cpu, bus);
}

void PatchPowerService(SceModule2 *mod)
{
	int i;
	for(i = 0; i < mod->text_size; i += 4)
	{
		u32 addr = mod->text_addr + i;
		if(_lw(addr) == 0x12030006)
		{
			u32 func = K_EXTRACT_CALL(addr + 0x1C);
			HIJACK_FUNCTION(func, SetClockPatched, SetClock);
		}
	}

	ClearCaches();
}