/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   RigEditor_IMain.h
	@date   08/2014
	@author Petr Ohlidal
*/

#pragma once

namespace RoR
{

namespace RigEditor
{

/** Command interface to RigEditor */
class IMain
{
public:

	virtual void CommandShowDialogOpenRigFile() = 0;

	virtual void CommandShowDialogSaveRigFileAs() = 0;

	virtual void CommandSaveRigFile() = 0;

	virtual void CommandCloseCurrentRig() = 0;
};

} // namespace RigEditor

} // namespace RoR
