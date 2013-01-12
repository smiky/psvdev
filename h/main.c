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

/* You can either run this code as binary loader. Or separate as binary file */
void _start() __attribute__((section(".text.start")));
void _start()
{
	SceUID (* _sceIoOpen)(const char *file, int flags, SceMode mode) = (void *)0xXXXXXXXX;
	int (* _sceIoRead)(SceUID fd, void *data, SceSize size) = (void *)0xXXXXXXXX;
	int (* _sceIoClose)(SceUID fd) = (void *)0xXXXXXXXX;

	/* Read TN.BIN to scratchpad */
	SceUID fd = _sceIoOpen("ms0:/PSP/SAVEDATA/.../TN.BIN", PSP_O_RDONLY, 0);
	_sceIoRead(fd, (void *)0x40010000, 16 * 1024);
	_sceIoClose(fd);

	/* args: length of savedata path without "TN.BIN" */
	/* argp: the path */
	/* Example: call(28, "ms0:/PSP/SAVEDATA/NPEZ00176/TN.BIN"); */
	void (* call)(SceSize args, void *argp) = (void *)0x00010000;
	call(..., "ms0:/PSP/SAVEDATA/.../TN.BIN");
}
