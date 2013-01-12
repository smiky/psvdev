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

#ifndef __CONF_H__
#define __CONF_H__

typedef struct
{
	char nickname[32];
	int button_assign;
	int cpu_speed;
	int iso_driver;
	int hide_cfw_folders;
	int exit_button_1;
	int exit_button_2;
	int exit_hold_duration;
	int show_pic1;
} TNConfig;

int sctrlSEGetConfig(TNConfig *config);

extern TNConfig conf;

#endif