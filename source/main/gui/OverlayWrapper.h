/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2019 Petr Ohlidal

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
/// @author Thomas Fischer
/// @date   6th of May 2010


#pragma once

#include "Application.h"

#include <Overlay/OgreTextAreaOverlayElement.h>
#include <OIS.h>

namespace RoR {

struct AeroEngineOverlay
{
    Ogre::OverlayElement *thr_element;
    Ogre::OverlayElement *engfire_element;
    Ogre::OverlayElement *engstart_element;
    Ogre::TextureUnitState *rpm_texture;
    Ogre::TextureUnitState *pitch_texture;
    Ogre::TextureUnitState *torque_texture;
};

struct AeroSwitchOverlay
{
    void Setup(std::string const & elem_name, std::string const & mat_on, std::string const & mat_off);
    void SetActive(bool value);

    Ogre::OverlayElement *element;
    Ogre::MaterialPtr on_material;
    Ogre::MaterialPtr off_material;
};

struct AeroTrimOverlay
{
    void Setup(std::string const & up, std::string const & dn, std::string const & disp);
    void DisplayFormat(const char* fmt, ...);

    Ogre::OverlayElement *up_button;
    Ogre::OverlayElement *dn_button;
    Ogre::OverlayElement *display;
};

struct AeroDashOverlay
{
    void SetThrottle(int engine, bool visible, float value);
    void SetEngineFailed(int engine, bool value);
    void SetEngineRpm(int engine, float pcent);
    void SetEnginePitch(int engine, float value);
    void SetEngineTorque(int engine, float pcent);
    void SetIgnition(int engine, bool visible, bool ignited);

    AeroEngineOverlay engines[4];

    Ogre::Overlay *dash_overlay;
    Ogre::Overlay *needles_overlay;

    Ogre::TextureUnitState *adibugstexture;
    Ogre::TextureUnitState *aditapetexture;
    Ogre::TextureUnitState *hsirosetexture;
    Ogre::TextureUnitState *hsibugtexture;
    Ogre::TextureUnitState *hsivtexture;
    Ogre::TextureUnitState *hsihtexture;
    Ogre::TextureUnitState *airspeedtexture;
    Ogre::TextureUnitState *altimetertexture;
    Ogre::TextureUnitState *vvitexture;
    Ogre::TextureUnitState *aoatexture;
    Ogre::TextAreaOverlayElement* alt_value_textarea;

    AeroSwitchOverlay hdg;
    AeroSwitchOverlay wlv;
    AeroSwitchOverlay nav;
    AeroSwitchOverlay alt;
    AeroSwitchOverlay vs;
    AeroSwitchOverlay ias;
    AeroSwitchOverlay gpws;
    AeroSwitchOverlay brks;

    AeroTrimOverlay hdg_trim;
    AeroTrimOverlay alt_trim;
    AeroTrimOverlay vs_trim;
    AeroTrimOverlay ias_trim;

    float thrust_track_top;
    float thrust_track_height;
};

class OverlayWrapper
{
public:

    OverlayWrapper();
    ~OverlayWrapper();

    struct LoadedOverlay
    {
        float orgScaleX = 0.f;
        float orgScaleY = 0.f;
        Ogre::Overlay *o = nullptr;
    };

    void ToggleDashboardOverlays(ActorPtr actor);

    void showDashboardOverlays(bool show, ActorPtr actor);

    void windowResized();
    void resizeOverlay(LoadedOverlay & overlay);

    bool mouseMoved(const OIS::MouseEvent& _arg);
    bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    

    void UpdatePressureOverlay(RoR::GfxActor* ga);
    void update(float dt);
    void UpdateLandVehicleHUD(RoR::GfxActor* ga);
    void UpdateAerialHUD(RoR::GfxActor* ga);
    void UpdateMarineHUD(ActorPtr vehicle);

    void ShowRacingOverlay();
    void HideRacingOverlay();
    void UpdateRacingGui(RoR::GfxScene* gs);

    Ogre::Overlay *loadOverlay(Ogre::String name, bool autoResizeRation=true);

protected:

    /// RoR needs to temporarily hide all overlays when player enters editor. 
    /// However, OGRE only provides per-overlay show() and hide() functionality.
    /// Thus, an external state must be kept to restore overlays after exiting the editor.
    struct VisibleOverlays
    {
        static const int RACING                       = BITMASK(4);
        static const int TRUCK_TIRE_PRESSURE_OVERLAY  = BITMASK(5);
    };

    int init();
    void resizePanel(Ogre::OverlayElement *oe);
    void reposPanel(Ogre::OverlayElement *oe);
    void placeNeedle(Ogre::SceneNode *node, float x, float y, float len);
    void updateStats(bool detailed=false);
    void showPressureOverlay(bool show);

    Ogre::OverlayElement *loadOverlayElement(Ogre::String name);

    Ogre::RenderWindow* win = nullptr;

    bool m_dashboard_visible = false;

    float mTimeUntilNextToggle = 0.f;

    // -------------------------------------------------------------
    // Overlays
    // -------------------------------------------------------------

    unsigned int  m_visible_overlays = 0;

    Ogre::Overlay *m_truck_pressure_overlay = nullptr;
    Ogre::Overlay *m_truck_pressure_needle_overlay = nullptr;

    AeroDashOverlay m_aerial_dashboard;

    Ogre::Overlay *m_marine_dashboard_overlay = nullptr;
    Ogre::Overlay *m_marine_dashboard_needles_overlay = nullptr;

    Ogre::Overlay *m_machine_dashboard_overlay = nullptr;

    // Misc
    Ogre::Overlay *m_racing_overlay = nullptr;

    // -------------------------------------------------------------
    // Overlay elements
    // -------------------------------------------------------------

    // Truck
    Ogre::OverlayElement* guiGear = nullptr;      //!< truck
    Ogre::OverlayElement* guiGear3D = nullptr;    //!< truck

    // Marine overlay elements
    Ogre::OverlayElement *bthro1 = nullptr;
    Ogre::OverlayElement *bthro2 = nullptr;

    // Truck
    Ogre::TextAreaOverlayElement* guiAuto[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    Ogre::TextAreaOverlayElement* guiAuto3D[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };

    // Truck (racing overlay)
    Ogre::TextAreaOverlayElement* laptime = nullptr;
    Ogre::TextAreaOverlayElement* bestlaptime = nullptr;

    Ogre::TextAreaOverlayElement* boat_depth_value_taoe = nullptr; //!< Marine

    // truck
    Ogre::TextureUnitState *speedotexture = nullptr; // Needed for dashboard prop
    Ogre::TextureUnitState *tachotexture = nullptr;  // Needed for dashboard prop

    // Marine
    Ogre::TextureUnitState *boatspeedtexture = nullptr;
    Ogre::TextureUnitState *boatsteertexture = nullptr;

    // Truck
    Ogre::TextureUnitState *pressuretexture = nullptr;

    // Marine: Written in init(), read-only in simulation.
    float thrtop = 0.f;
    float thrheight = 0.f;
    float throffset = 0.f;

    std::vector<LoadedOverlay> m_loaded_overlays;
};

} // namespace RoR
