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
/*! @mainpage

	have fun coding :)

	please note that the documentation is work in progress
*/
#ifndef RIGSOFRODS_H__
#define RIGSOFRODS_H__

#include "RoRPrerequisites.h"

#include "AdvancedOgreFramework.h"
#include "AppStateManager.h"

class RigsOfRods : public RoRSingletonNoCreation<RigsOfRods>, public ZeroedMemoryAllocator
{
public:
	RigsOfRods(Ogre::String name = Ogre::String("RoR"), Ogre::String hwnd=Ogre::String(), Ogre::String mainhwnd=Ogre::String(), bool embedded = false);
	~RigsOfRods();

	void go();
	void pauseRendering(void);
	void shutdown(void);
	void tryShutdown(void);

	void update(double dt);
protected:
	AppStateManager *stateManager;
	Ogre::String hwnd, mainhwnd;
	Ogre::String name;
	bool embedded;
};

#endif //RIGSOFRODS_H__
