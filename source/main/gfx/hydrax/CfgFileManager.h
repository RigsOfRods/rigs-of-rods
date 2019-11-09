/*
--------------------------------------------------------------------------------
This source file is part of Hydrax.
Visit ---

Copyright (C) 2008 Xavier Vergu�n Gonz�lez <xavierverguin@hotmail.com>
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

#ifndef _Hydrax_CfgFileManager_H_
#define _Hydrax_CfgFileManager_H_

#include "Help.h"
#include "Prerequisites.h"

namespace Hydrax
{
    class Hydrax;

    /** Config file manager.
        Class to load/save all Hydrax options from/to a config file
     */
    class CfgFileManager
    {
      public:
        /** Constructor
            @param h Hydrax parent pointer
         */
        CfgFileManager(Hydrax *h);

        /** Destructor
         */
        ~CfgFileManager();

        /** Load hydrax cfg file
            @param File File name
            @return false if an error has been ocurred(Check the log file in this case).
         */
        const bool load(const Ogre::String &File) const;

        /** Save current hydrax config to a file
            @param File Destination file name
            @param Path File path
            @return false if an error has been ocurred(Check the log file in this case).
         */
        const bool save(const Ogre::String &File, const Ogre::String &Path = "") const;

        /** <int> Get the cfg string
            @param Name Parameter name
            @param Value Parameter value
            @return int cfg string
         */
        static Ogre::String _getCfgString(const Ogre::String &Name, const int &Value);

        /** <float> Get the cfg string
            @param Name Parameter name
            @param Value Parameter value
            @return Ogre::Real cfg string
         */
        static Ogre::String _getCfgString(const Ogre::String &Name, const Ogre::Real &Value);

        /** <bool> Get the cfg string
            @param Name Parameter name
            @param Value Parameter value
            @return bool cfg string
         */
        static Ogre::String _getCfgString(const Ogre::String &Name, const bool &Value);

        /** <vector2> Get the cfg string
            @param Name Parameter name
            @param Value Parameter value
            @return Ogre::Vector2 cfg string
         */
        static Ogre::String _getCfgString(const Ogre::String &Name, const Ogre::Vector2 &Value);

        /** <vector3> Get the cfg string
            @param Name Parameter name
            @param Value Parameter value
            @return Ogre::Vector3 cfg string
         */
        static Ogre::String _getCfgString(const Ogre::String &Name, const Ogre::Vector3 &Value);

        /** <size> Get the cfg string
            @param Name Parameter name
            @param Value Parameter value
            @return Hydrax::Size cfg string
         */
        static Ogre::String _getCfgString(const Ogre::String &Name, const Size &Value);

        /** Get int value
            @param CfgFile Config file
            @param Name Parameter name
            @return int value
            @remarks if the parameter isn't found or the data type is not an int value, return 0 as default
         */
        static int _getIntValue(Ogre::ConfigFile &CfgFile, const Ogre::String Name);

        /** Get float value
            @param CfgFile Config file
            @param Name Parameter name
            @return float value
            @remarks if the parameter isn't found or the data type is not a float value, return 0 as default
         */
        static Ogre::Real _getFloatValue(Ogre::ConfigFile &CfgFile, const Ogre::String Name);

        /** Get bool value
            @param CfgFile Config file
            @param Name Parameter name
            @return bool value
            @remarks if the parameter isn't found or the data type is not a bool value, return false as default
         */
        static bool _getBoolValue(Ogre::ConfigFile &CfgFile, const Ogre::String Name);

        /** Get vector2 value
            @param CfgFile Config file
            @param Name Parameter name
            @return vector2 value
            @remarks if the parameter isn't found or the data type is not an int value, returns (0,0) as default
         */
        static Ogre::Vector2 _getVector2Value(Ogre::ConfigFile &CfgFile, const Ogre::String Name);

        /** Get vector3 value
            @param CfgFile Config file
            @param Name Parameter name
            @return vector3 value
            @remarks if the parameter isn't found or the data type is not an int value, returns (0,0,0) as default
         */
        static Ogre::Vector3 _getVector3Value(Ogre::ConfigFile &CfgFile, const Ogre::String Name);

        /** Get size value
            @param CfgFile Config file
            @param Name Parameter name
            @return size value
            @remarks if the parameter isn't found or the data type is not an int value, returns (0,0) as default
         */
        static Size _getSizeValue(Ogre::ConfigFile &CfgFile, const Ogre::String Name);

        /** Check is a std::vector<Ogre::String> contains a specified Ogre::String
            @param List String list
            @param Find String to find
            @return true if it's contained, false if not
         */
        static bool _isStringInList(const Ogre::StringVector &List, const Ogre::String &Find);

      private:
        /** Save a string in file
            @param Data Data
            @param File Destination file
            @param Path File path
            @return false if an error has ocurred
         */
        const bool _saveToFile(const Ogre::String &Data, const Ogre::String &File, const Ogre::String &Path) const;

        /** Load a cfg file in an Ogre::ConfigFile
            @param File File name
            @param Result, std::pair<bool, Ogre::ConfigFile&> First: False if the file isn't in the Hydrax resource group, Second:
           Ogre::ConfigFile
         */
        const void _loadCfgFile(const Ogre::String &File, std::pair<bool, Ogre::ConfigFile> &Result) const;

        /** Get components config string
            @return Components cfg string
         */
        const Ogre::String _getComponentsCfgString() const;

        /** Load components settings
            @param CfgFile Config file
         */
        const void _loadComponentsSettings(Ogre::ConfigFile &CfgFile) const;

        /** Get rtt quality config string
            @return Rtt quality cfg string
         */
        const Ogre::String _getRttCfgString() const;

        /** Load rtt settings
            @param CfgFile Config file
         */
        const void _loadRttSettings(Ogre::ConfigFile &CfgFile) const;

        /** Get hydrax version cfg string
            @return Hydrax version cfg string
         */
        const Ogre::String _getVersionCfgString() const;

        /** Check hydrax version cfg file
            @param CfgFile Config file
            @return true if it's the same version, false if not.
         */
        const bool _checkVersion(Ogre::ConfigFile &CfgFile) const;

        /// Hydrax parent pointer
        Hydrax *mHydrax;
    };
}; // namespace Hydrax

#endif
