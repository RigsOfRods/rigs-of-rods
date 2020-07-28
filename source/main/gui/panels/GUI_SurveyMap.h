/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

#pragma once

/// @file

#include "Application.h"

#include "OgreImGui.h"
#include "SurveyMapTextureCreator.h"

namespace RoR {
namespace GUI {

/// In-game map widget
/// Has 3 display modes (see `SurveyMapMode`), cycled using input `SURVEY_MAP_TOGGLE_VIEW`
///  * NONE - not visible
///  * SMALL - 30% of window height, zoomable with inputs `SURVEY_MAP_ZOOM_[IN|OUT]`
///  * BIG - 98% of window height, not zoomable
/// Maintains 2 textures:
///  * static terrain texture (rendered on session start, used in BIG+SMALL map on full zoom-out)
///  * dynamic texture, used in SMALL map when zoomed-in
/// Settings:
///  * gfx_surveymap_icons - Disables icons, toggle with input `EV_SURVEY_MAP_TOGGLE_ICONS`
///  * gfx_declutter_map - Hides icon captions (terrain object names+types, telepoint names, MP usernames)
class SurveyMap
{
public:

    const float WINDOW_PADDING = 4.f;
    const float WINDOW_ROUNDING = 2.f;

    void CreateTerrainTextures(); //!< Init
    void Draw();
    bool IsVisible() const { return mMapMode != SurveyMapMode::NONE; }
    void CycleMode();
    void ToggleMode();

    const char* getTypeByDriveable(int driveable);

protected:

    enum class SurveyMapMode
    {
        NONE, // Not visible
        SMALL,
        BIG
    };

    void setMapZoom(float zoom);
    void setMapZoomRelative(float dt_sec);

    void DrawMapIcon(ImVec2 view_pos, ImVec2 view_size, Ogre::Vector2 view_origin,
                     std::string const& filename, std::string const& caption, 
                     float pos_x, float pos_y, float angle);

    SurveyMapMode mMapMode = SurveyMapMode::NONE; // Display mode
    SurveyMapMode mMapLastMode = SurveyMapMode::NONE; // Display mode
    Ogre::Vector2 mTerrainSize = Ogre::Vector2::ZERO; // Computed reference map size (in meters)
    Ogre::Vector2 mMapCenterOffset = Ogre::Vector2::ZERO; // Displacement, in meters
    float         mMapZoom = 0.f; // Ratio: 0-1

    std::unique_ptr<SurveyMapTextureCreator> mMapTextureCreatorStatic;
    std::unique_ptr<SurveyMapTextureCreator> mMapTextureCreatorDynamic;
};

} // namespace GUI
} // namespace RoR

