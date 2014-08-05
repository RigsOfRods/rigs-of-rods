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
// created on 15th of May 2011 by Thomas Fischer

#ifndef CBYTECODEESTREAM_H__
#define CBYTECODEESTREAM_H__

#include "RoRPrerequisites.h"

#include <angelscript.h>

class CBytecodeStream : public AngelScript::asIBinaryStream
{
public:
	CBytecodeStream(std::string filename);
	~CBytecodeStream();
	void Read(void *ptr, AngelScript::asUINT size);
	void Write(const void *ptr, AngelScript::asUINT size);
	bool Existing();
private:
	FILE *f;
};

#endif //CBYTECODEESTREAM_H__