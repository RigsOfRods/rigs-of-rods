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
// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 9th of August 2009

#ifndef __UTILS_H_
#define __UTILS_H_

#include "RoRPrerequisites.h"

#include <OgreDataStream.h>

#ifdef USE_MYGUI
#include <MyGUI.h>
#endif //USE_MYGUI

#include <pthread.h>

#if WIN32
# include <windows.h> // Sleep()
#endif // WIN32


// from http://stahlforce.com/dev/index.php?tool=csc01
Ogre::String hexdump(void *pAddressIn, long  lSize);

Ogre::UTFString tryConvertUTF(const char *buffer);

Ogre::UTFString formatBytes(double bytes);


Ogre::String getASCIIFromCharString(char *str, int maxlen);
Ogre::String getASCIIFromOgreString(Ogre::String s, int maxlen);

int getTimeStamp();

Ogre::String getVersionString(bool multiline = true);

bool fileExists(const char *filename);
bool folderExists(const char *pathname);

int isPowerOfTwo(unsigned int x);

Ogre::String stripNonASCII(Ogre::String s);

Ogre::AxisAlignedBox getWorldAABB(Ogre::SceneNode* node);

void fixRenderWindowIcon (Ogre::RenderWindow* rw);

std::wstring ANSI_TO_WCHAR(const Ogre::String source);
Ogre::UTFString ANSI_TO_UTF(const Ogre::String source);

void trimUTFString(Ogre::UTFString &str, bool left = true, bool right = true);

#ifdef USE_MYGUI
inline MyGUI::UString convertToMyGUIString(char *charstr)
{
	return MyGUI::UString(ANSI_TO_WCHAR(charstr));
}

inline MyGUI::UString convertToMyGUIString(wchar_t *charstr, int len)
{
	return MyGUI::UString(charstr, len);
}

inline MyGUI::UString convertToMyGUIString(std::wstring str)
{
	return MyGUI::UString(str);
}

inline MyGUI::UString convertToMyGUIString(Ogre::UTFString str)
{
	return MyGUI::UString(str.asWStr());
}
inline Ogre::UTFString convertFromMyGUIString(MyGUI::UString str)
{
	return Ogre::UTFString(str.asWStr());
}
#endif // USE_MYGUI


inline void replaceString(std::string &str, std::string searchString, std::string replaceString)
{
	std::string::size_type pos = 0;
	while ( (pos = str.find(searchString, pos)) != std::string::npos )
	{
		str.replace( pos, searchString.size(), replaceString );
		pos++;
	}
}

inline void sleepMilliSeconds(unsigned int ms)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	// accepts milliseconds
	Sleep(ms);
#else
	// accepts microseconds
	usleep(ms * 1000);
#endif
}

inline unsigned long getThreadID()
{
#ifdef WIN32
	return (unsigned long)pthread_self().p;
#else
	return (unsigned long)pthread_self();
#endif // WIN32
}

inline Ogre::String getThreadIDAsString()
{
	return TOSTRING(getThreadID());
}

Ogre::Real Round(Ogre::Real value, unsigned short ndigits = 0);

// generates a hash from a DataStream, beware, its loading the whole thing into a buffer, so its not suited for big files
void generateHashFromDataStream(Ogre::DataStreamPtr &ds, Ogre::String &hash);
void generateHashFromFile(Ogre::String filename, Ogre::String &hash);

#endif // __UTILS_H_
