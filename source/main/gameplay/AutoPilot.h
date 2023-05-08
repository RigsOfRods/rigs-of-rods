/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

#include "Application.h"
#include "TerrainObjectManager.h"

namespace RoR {

/// @addtogroup Gameplay
/// @{

/// @addtogroup Aerial
/// @{

class Autopilot
{
public:

    enum
    {
        HEADING_NONE,
        HEADING_FIXED,
        HEADING_WLV,
        HEADING_NAV
    };

    enum
    {
        ALT_NONE,
        ALT_FIXED,
        ALT_VS
    };

    int heading;
    bool wantsdisconnect;

    Autopilot(int actor_id);
    void reset();
    void disconnect();
    void setInertialReferences(node_t* refl, node_t* refr, node_t* refb, node_t* refc);
    int toggleHeading(int mode);
    int toggleAlt(int mode);
    bool toggleIAS();
    bool toggleGPWS();
    int adjHDG(int d);
    int adjALT(int d);
    int adjVS(int d);
    int adjIAS(int d);

    float getAilerons();
    float getElevator();
    float getRudder();
    float getThrottle(float thrtl, float dt);

    void gpws_update(float spawnheight);

    void UpdateIls(std::vector<RoR::TerrainObjectManager::localizer_t> localizers);
    float GetVerticalApproachDeviation() { return m_ils_angle_vdev; }
    float GetHorizontalApproachDeviation() { return m_ils_angle_hdev; }
    bool IsIlsAvailable() { return m_horizontal_locator_available && m_vertical_locator_available; }
    int GetHeadingMode() const { return mode_heading; }
    int GetAltMode() const { return mode_alt; }
    int GetAltValue() const { return alt; }
    bool GetIasMode() const { return mode_ias; }
    int GetIasValue() const { return ias; }
    bool GetGpwsMode() const { return mode_gpws; }
    int GetVsValue() const { return vs; }
private:

    int mode_heading;
    int mode_alt;
    bool mode_ias;
    bool mode_gpws;
    int alt;
    int vs;
    int ias;
    node_t* ref_l;
    node_t* ref_r;
    node_t* ref_b;
    node_t* ref_c;
    float ref_span;
    float last_elevator;
    float last_aileron;
    float last_rudder;
    float last_gpws_height;
    float last_pullup_height;

    bool m_vertical_locator_available;
    bool m_horizontal_locator_available;
    float m_ils_angle_vdev;
    float m_ils_angle_hdev;
    float m_ils_runway_heading;
    float m_ils_runway_distance;
    float last_closest_hdist;

    int m_actor_id;
};

/// @} // addtogroup Aerial
/// @} // addtogroup Gameplay

} // namespace RoR
