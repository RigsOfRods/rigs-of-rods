/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __TwoDReplay_H_
#define __TwoDReplay_H_

#include <vector>
#include <stdio.h>

/** FILE FORMAT
 *
 *  HEADER - r2d_file_header_t
 *  RIG - r2d_file_rig_t
 *   WHEELSETUP - r2d_file_wheel_setup_t
 *   WHEELSETUP
 *  RIG
 *   ...
 *
 *  ENTRY - r2d_file_entry_t
 *   FRAME - r2d_file_frame_t
 *    WHEELFRAME - r2d_file_wheel_frame_t
 *    WHEELFRAME
 *  ENTRY
 *   EVENTINFO
 *  ENTRY
 *   ...
 */

#define R2DFILEFORMAT "ROR2DV1"

typedef struct r2d_file_header_t
{
	char fileformat[8];
	int num_rigs;
} r2d_file_header_t;

typedef struct r2d_file_rig_t
{
	char num;
	char name[255];
	char filename[255];
	char hash[40];
	char num_wheels;
} r2d_file_rig_t;

typedef struct r2d_file_wheel_setup_t
{
	float width;
	float radius;
	char  type;
} r2d_file_wheel_setup_t;


typedef struct r2d_file_entry_t
{
	char type;
	char rignum;
	unsigned long int time_ms;
	unsigned long int physframe;
} r2d_file_entry_t;

typedef struct r2d_file_frame_t
{
	float brake;
	float steering;
	float accel;
	float clutch;
	char gear;
	char gearMode;
	bool pbrake;
} r2d_file_frame_t;

typedef struct r2d_file_wheel_frame_t
{
	float wheel_pos[3];
	float wheel_direction;
	float wheel_slip;
	float wheel_speed;
} r2d_file_wheel_frame_t;

typedef struct r2d_memory_structure_rig_t
{
	r2d_file_entry_t entry;
	std::vector<r2d_file_frame_t> frames;
	std::vector<r2d_file_wheel_frame_t> wframes;
} r2d_memory_structure_rig_t;

enum { R2DTYPE_NULL, R2DTYPE_FRAME, R2DTYPE_EVENT, R2DTYPE_END };

class TwoDReplay
{
protected:

	FILE *file;
	float time;

	void start();

public:

	TwoDReplay();
	~TwoDReplay();

	void update(float &dt);
	void recordFrame();
};

#endif // __TwoDReplay_H_
