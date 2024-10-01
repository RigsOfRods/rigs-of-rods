/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2016 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/**
    @file
    @author Thomas Fischer thomas{AT}thomasfischer{DOT}biz
    @date   9th of August 2009
*/

#pragma once

#include "Application.h"

#include "utf8/checked.h"
#include "utf8/unchecked.h"

#include <MyGUI.h>
#include <OgreUTFString.h>

namespace RoR {

Ogre::String sha1sum(const char *key, int len);

Ogre::String HashData(const char *key, int len);

Ogre::UTFString tryConvertUTF(const char* buffer);

Ogre::UTFString formatBytes(double bytes);

std::time_t getTimeStamp();

Ogre::String getVersionString(bool multiline = true);

inline void replaceString(std::string& str, std::string searchString, std::string replaceString)
{
    std::string::size_type pos = 0;
    while ((pos = str.find(searchString, pos)) != std::string::npos)
    {
        str.replace(pos, searchString.size(), replaceString);
        pos++;
    }
}

Ogre::Real Round(Ogre::Real value, unsigned short ndigits = 0);

std::string SanitizeUtf8String(std::string const& str_in);
std::string SanitizeUtf8CString(const char* start, const char* end = nullptr);

inline std::string& TrimStr(std::string& s) { Ogre::StringUtil::trim(s); return s; }
std::string Sha1Hash(std::string const & data);

std::string JoinStrVec(Ogre::StringVector tokens, const std::string& delim);

// for std::vector
template <class T, class A, class Predicate>
inline void EraseIf(std::vector<T, A>& c, Predicate pred)
{
    c.erase(std::remove_if(c.begin(), c.end(), pred), c.end());
}

/// @author http://www.ogre3d.org/forums/viewtopic.php?p=463232#p463232
/// @author http://www.ogre3d.org/tikiwiki/tiki-index.php?page=GetScreenspaceCoords&structure=Cookbook
class World2ScreenConverter ///< Keeps data close for faster access.
{
public:
    World2ScreenConverter(Ogre::Matrix4 view_mtx, Ogre::Matrix4 proj_mtx, Ogre::Vector2 screen_size):
        m_view_matrix(view_mtx), m_projection_matrix(proj_mtx), m_screen_size(screen_size)
    {}

    /// @return X,Y=screen pos, Z=view space pos ('Z<0' means 'in front of the camera')
    Ogre::Vector3 Convert(Ogre::Vector3 world_pos)
    {
        Ogre::Vector3 view_space_pos = m_view_matrix * world_pos;
        Ogre::Vector3 clip_space_pos = m_projection_matrix * view_space_pos; // Clip space pos is [-1.f, 1.f]
        float screen_x = ((clip_space_pos.x / 2.f) + 0.5f) * m_screen_size.x;
        float screen_y = (1.f - ((clip_space_pos.y / 2.f) + 0.5f)) * m_screen_size.y;
        return Ogre::Vector3(screen_x, screen_y, view_space_pos.z);
    }

private:
    Ogre::Matrix4 m_view_matrix;
    Ogre::Matrix4 m_projection_matrix;
    Ogre::Vector2 m_screen_size;
};

bool IsDistanceWithin(Ogre::Vector3 const& a, Ogre::Vector3 const& b, float max);

std::string PrintMeshInfo(std::string const& title, Ogre::MeshPtr mesh);

void CvarAddFileToList(CVar* cvar, const std::string& filename);
void CvarRemoveFileFromList(CVar* cvar, const std::string& filename);

void SplitBundleQualifiedFilename(const std::string& bundleQualifiedFilename, std::string& out_bundleName, std::string& out_filename);

} // namespace RoR
