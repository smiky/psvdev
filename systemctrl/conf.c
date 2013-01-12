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

#include "conf.h"

TNConfig conf;

int sctrlSESetConfig(TNConfig *config)
{
	int k1 = pspSdkSetK1(0);

	sceIoMkdir("ms0:/PSP/SYSTEM", 0777);
	SceUID fd = sceIoOpen("ms0:/PSP/SYSTEM/CONFIG.TN", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
	if(fd < 0)
	{
		pspSdkSetK1(k1);
		return fd;
	}

	memcpy(&conf, config, sizeof(TNConfig));

	int written = sceIoWrite(fd, config, sizeof(TNConfig));
	sceIoClose(fd);

	pspSdkSetK1(k1);
	return written;
}

int sctrlSEGetConfig(TNConfig *config)
{
	int k1 = pspSdkSetK1(0);

	memset(config, 0, sizeof(TNConfig));

	/* Default settings */
	strcpy(config->nickname, "CEF User");
	config->exit_button_1 = 2; //PSP_CTRL_START
	config->exit_hold_duration = 2;
	config->button_assign = 1; //CROSS
	config->show_pic1 = 1; //enabled

	SceUID fd = sceIoOpen("ms0:/PSP/SYSTEM/CONFIG.TN", PSP_O_RDONLY, 0);
	if(fd < 0)
	{
		pspSdkSetK1(k1);
		return fd;
	}

	int read = sceIoRead(fd, config, sizeof(TNConfig));
	sceIoClose(fd);

	pspSdkSetK1(k1);
	return read;
}