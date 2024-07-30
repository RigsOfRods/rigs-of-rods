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

#pragma once

/// @file

#include "Application.h"
#include "OgreImGui.h"
#include "SimData.h"
#include "SurveyMapEntity.h"
#include "SurveyMapTextureCreator.h"

#include <vector>

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

    void CreateTerrainTextures();
    void Draw();
    bool IsVisible() const { return mMapMode != SurveyMapMode::NONE; }
    bool IsHovered() const { return IsVisible() && mWindowMouseHovered; }
    void CycleMode();
    void ToggleMode();

protected:

    enum class SurveyMapMode
    {
        NONE, // Not visible
        SMALL,
        BIG
    };

    void setMapZoom(float zoom);
    void setMapZoomRelative(float dt);
    const char* getTypeByDriveable(const ActorPtr& actor);
    const char* getAIType(const ActorPtr& actor);

    void CacheMapIcon(SurveyMapEntity& e);

    void DrawMapIcon(const SurveyMapEntity& e, ImVec2 view_pos, ImVec2 view_size, Ogre::Vector2 view_origin);

    ImVec2 DrawWaypoint(ImVec2 view_pos, ImVec2 view_size, Ogre::Vector2 view_origin,
                     std::string const& caption, int idx);

    ImVec2 CalcWaypointMapPos(ImVec2 view_pos, ImVec2 view_size, Ogre::Vector2 view_origin, int idx);

    // Window display
    SurveyMapMode mMapMode = SurveyMapMode::NONE; // Display mode
    SurveyMapMode mMapLastMode = SurveyMapMode::NONE; // Display mode
    bool          mWindowMouseHovered = false;
    bool          mMouseClicked = false;
    int           mWaypointNum = 0;

    // Map
    Ogre::Vector2 mTerrainSize = Ogre::Vector2::ZERO; // Computed reference map size (in meters)
    Ogre::Vector2 mMapCenterOffset = Ogre::Vector2::ZERO; // Displacement, in meters
    float         mMapZoom = 0.f; // Ratio: 0-1
    Ogre::TexturePtr mMapTexture;

    // Icon cache
    bool m_icons_cached = false;
    Ogre::TexturePtr m_left_mouse_button;
    Ogre::TexturePtr m_middle_mouse_button;
    Ogre::TexturePtr m_middle_mouse_scroll_button;
    Ogre::TexturePtr m_right_mouse_button;
    void CacheIcons();

    // Circular minimap
    ImVec2 m_circle_center;
    float m_circle_radius = 0.f;
};

} // namespace GUI
} // namespace RoR
