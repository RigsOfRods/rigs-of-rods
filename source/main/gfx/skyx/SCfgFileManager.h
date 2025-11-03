/*
--------------------------------------------------------------------------------
This source file is part of Hydrax.
Visit ---

Copyright (C) 2008 Xavier Verguín González <xavierverguin@hotmail.com>
                                           <xavyiy@gmail.com>

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
--------------------------------------------------------------------------------
*/

#ifndef _SkyX_CfgFileManager_H_
#define _SkyX_CfgFileManager_H_

#include "Prerequisites.h"

#include "Help.h"

namespace SkyX
{
	class SkyX;
	class BasicController;

	/** Config file manager.
	    Class to load/save all Hydrax options from/to a config file
	 */
	class CfgFileManager
	{
	public:
		/** Constructor
		    @param h Hydrax parent pointer
		 */
		CfgFileManager(SkyX *s, BasicController *c, Ogre::Camera* d);

		/** Destructor
		 */
        ~CfgFileManager();

		/** Load hydrax cfg file
		    @param File File name
			@return false if an error has been ocurred(Check the log file in this case).
		 */
		const bool load(const Ogre::String& File) const;

		/** Save current hydrax config to a file
		    @param File Destination file name
			@param Path File path
			@return false if an error has been ocurred(Check the log file in this case).
		 */
		const bool save(const Ogre::String& File, const Ogre::String& Path = "") const;

		static Ogre::String _getCfgString(const Ogre::String& Name, const int& Value);
		static Ogre::String _getCfgString(const Ogre::String& Name, const Ogre::Real& Value);
		static Ogre::String _getCfgString(const Ogre::String& Name, const bool& Value);
		static Ogre::String _getCfgString(const Ogre::String& Name, const Ogre::Vector2& Value);
		static Ogre::String _getCfgString(const Ogre::String& Name, const Ogre::Vector3& Value);
		static Ogre::String _getCfgString(const Ogre::String &Name, const Ogre::Vector4 &Value);
		static Ogre::String _getCfgString(const Ogre::String &Name, const Ogre::Degree &Value);

		/** Get int value
			@remarks if the parameter isn't found or the data type is not an int value, return 0 as default
		 */
		static int _getIntValue(Ogre::ConfigFile& CfgFile, const Ogre::String Name);

		/** Get float value
			@remarks if the parameter isn't found or the data type is not a float value, return 0 as default
		 */
		static Ogre::Real _getFloatValue(Ogre::ConfigFile& CfgFile, const Ogre::String Name);

		/** Get bool value
			@remarks if the parameter isn't found or the data type is not a bool value, return false as default
		 */
		static bool _getBoolValue(Ogre::ConfigFile& CfgFile, const Ogre::String Name);

		/** Get vector2 value
			@remarks if the parameter isn't found or the data type is not an int value, returns (0,0) as default
		 */
		static Ogre::Vector2 _getVector2Value(Ogre::ConfigFile& CfgFile, const Ogre::String Name);

		/** Get vector3 value
			@remarks if the parameter isn't found or the data type is not an int value, returns (0,0,0) as default
		 */
		static Ogre::Vector3 _getVector3Value(Ogre::ConfigFile& CfgFile, const Ogre::String Name);

		static Ogre::Vector4 _getVector4Value(Ogre::ConfigFile& CfgFile, const Ogre::String Name);

		static Ogre::Degree _getDegreeValue(Ogre::ConfigFile& CfgFile, const Ogre::String Name);

		static bool _isStringInList(const Ogre::StringVector &List, const Ogre::String &Find);

	private:
		/** Save a string in file
			@return false if an error has ocurred
		 */
		const bool _saveToFile(const Ogre::String& Data, const Ogre::String& File, const Ogre::String& Path) const;

		/** Load a cfg file in an Ogre::ConfigFile
		    @param File File name
			@param Result, std::pair<bool, Ogre::ConfigFile&> First: False if the file isn't in the Hydrax resource group, Second: Ogre::ConfigFile
		 */
		const void _loadCfgFile(const Ogre::String& File, std::pair<bool,Ogre::ConfigFile> &Result) const;

		/** Get hydrax version cfg string
		    @return Hydrax version cfg string
		 */
		const Ogre::String _getVersionCfgString() const;

		/** Check hydrax version cfg file
		    @param CfgFile Config file
		    @return true if it's the same version, false if not.
		 */
		const bool _checkVersion(Ogre::ConfigFile& CfgFile) const;

		/// Hydrax parent pointer
		SkyX* mSkyX;
		BasicController* mController;
		Ogre::Camera *mCamera;
	};
};

#endif
