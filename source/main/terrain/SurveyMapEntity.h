/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2023 Petr Ohlidal

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

/// @file

#pragma once

#include <Ogre.h>
#include <string>
#include <vector>

namespace RoR {

    /// @addtogroup Terrain
    /// @{

    struct SurveyMapEntity
    {
        std::string type; //!< informational
        std::string caption; //!< display caption
        std::string filename; //!< Requested icon, cached may be out of date.
        std::string resource_group; //!< if empty, defaults to TexturesRG
        Ogre::Vector3 pos; //!< world pos in meters
        Ogre::Radian rot_angle; //!< world yaw in radians
        int id; //!< race ID (>=0), or -1 if not a race icon. You can use larger negative numbers for custom IDs.
        Ogre::TexturePtr cached_icon;
        bool draw_caption = false; //!< By default, don't draw caption under icon, use the mouse tooltip.
        Ogre::ColourValue caption_color = Ogre::ColourValue::White;
    };

    typedef std::vector<SurveyMapEntity> SurveyMapEntityVec;

    /// @} // addtogroup Terrain

} // namespace RoR
