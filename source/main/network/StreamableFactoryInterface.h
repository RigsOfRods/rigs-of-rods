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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 22th of December 2009

#ifndef __StreamableFactoryInterface_H_
#define __StreamableFactoryInterface_H_

#include "RoRPrerequisites.h"

#include "rornet.h"

class Streamable;

typedef struct stream_reg_t
{
	int sourceid;
	int streamid;
	stream_register_t reg;
	int colour;
} stream_reg_t;

typedef struct stream_del_t
{
	int sourceid;
	int streamid;
} stream_del_t;

class StreamableFactoryInterface
{
public:

	virtual void netUserAttributesChanged(int source, int streamid) = 0;
	virtual Streamable *createRemoteInstance(stream_reg_t *reg) = 0;
	virtual void createRemote(int sourceid, int streamid, stream_register_t *reg, int colour) = 0;
	virtual void deleteRemote(int sourceid, int streamid) = 0;
	virtual bool syncRemoteStreams() = 0;
	virtual void removeInstance(stream_del_t *del) = 0;
};

#endif // __StreamableFactoryInterface_H_
