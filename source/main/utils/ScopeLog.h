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

#ifndef SCOPELOG_H
#define SCOPELOG_H

#include "RoRPrerequisites.h"


#include <OgreLog.h>

/**
 * @brief: this class will change the default log with the scope of its creation.
 * Upon scope leaving, the previous default Log is restored.
 */
class ScopeLog : Ogre::LogListener, public ZeroedMemoryAllocator
{
protected:
	Ogre::Log *orgLog;
	FILE *f;
	unsigned int counter;
	bool disabled;
	bool headerWritten;
	Ogre::String logFileName, name;
	int getFileSize(Ogre::String filename);
	// method from Ogre::LogListener
#if OGRE_VERSION < ((1 << 16) | (8 << 8 ) | 0)
	void messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName);
#else
	void messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName, bool& skipThisMessage);
#endif // OGRE_VERSION

public:
	ScopeLog(Ogre::String filename);
	~ScopeLog();

	int warning, error, obsoleteWarning, info;
};

#endif
