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
	@file   RigEditor_RigFactory.h
	@date   06/2014
	@author Petr Ohlidal
*/

#pragma once

#include "RigDef_Prerequisites.h"
#include "RoRPrerequisites.h"
#include "RigDef_File.h"

namespace RoR
{

namespace RigEditor
{

class RigFactory
{
public:

	RigFactory()
	{}

	Rig* BuildRig(RigDef::File* rig_def, std::vector<RigDef::File::Module*> & selected_modules, RigEditor::Main* rig_editor);

private:

	void AddMessage(RigDef::File::Module* module, std::string const & text);

	std::vector<Ogre::String> m_messages;
};

} // namespace RigEditor

} // namespace RoR
