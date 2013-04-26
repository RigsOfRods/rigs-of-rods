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
//created by thomas fischer 23 February 2009
#ifndef __SAVEGAME_H_
#define __SAVEGAME_H_

#include "RoRPrerequisites.h"

class Savegame : public ZeroedMemoryAllocator
{
public:

	int load(Ogre::String &filename);
	int save(Ogre::String &filename);

protected:

	static const char *current_version;
	static const int  entry_magic = 0xCAFED00D;
	
	// data structures for the file format below
	// Rigs of Rods savegame file format:
	
	// savegame_header
	// savegame_entry_header
	// DATA
	// savegame_entry_header
	// DATA
	// etc ...

	struct savegame_header {
		char savegame_version[28];
		char ror_version[20];
		unsigned int entries;
		int current_truck;
		// character, TODO: animations
		float player_pos[3];
		// camera
		float cam_pos[3];
		float cam_ideal_pos[3];
		float camRotX, camRotY, camDist;
		float pushcamRotX, pushcamRotY;
		float mMoveScale, mRotScale;
		float lastPosition[3];
		int cameramode, lastcameramode;
	};

	struct savegame_entry_header {
		unsigned int magic;
		char filename[1024];
		unsigned int free_nodes;
		unsigned int free_beams;
		unsigned int free_shock;
		unsigned int free_wheel;
		unsigned int free_hooks;
		unsigned int free_rotator;
		unsigned int state;
		unsigned int engine;
		unsigned int collisionBoundingBoxeses;
		float origin[3];
	};
};

#endif // __SAVEGAME_H_
