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

#include "CBytecodeStream.h"

CBytecodeStream::CBytecodeStream(std::string filename) : f(0)
{
	f = fopen(filename.c_str(), "wb");
}

CBytecodeStream::~CBytecodeStream()
{
	if (f)
		fclose(f);
}

void CBytecodeStream::Write(const void *ptr, AngelScript::asUINT size)
{
	if (!f) return;
	fwrite(ptr, size, 1, f);
}

void CBytecodeStream::Read(void *ptr, AngelScript::asUINT size)
{
	if (!f) return;
	fread(ptr, size, 1, f);
}

bool CBytecodeStream::Existing()
{
	return (f != 0);
}
