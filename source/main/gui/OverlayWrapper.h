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

#include <OgreTextAreaOverlayElement.h>
#include <OIS.h>

namespace RoR {

struct AeroEngineOverlay
{
    Ogre::v1::OverlayElement *thr_element;
    Ogre::v1::OverlayElement *engfire_element;
    Ogre::v1::OverlayElement *engstart_element;
    Ogre::TextureUnitState *rpm_texture;
    Ogre::TextureUnitState *pitch_texture;
    Ogre::TextureUnitState *torque_texture;
};

struct AeroSwitchOverlay
{
    void Setup(std::string const & elem_name, std::string const & mat_on, std::string const & mat_off);
    void SetActive(bool value);

    Ogre::v1::OverlayElement *element;
    Ogre::MaterialPtr on_material;
    Ogre::MaterialPtr off_material;
};

struct AeroTrimOverlay
{
    void Setup(std::string const & up, std::string const & dn, std::string const & disp);
    void DisplayFormat(const char* fmt, ...);

    Ogre::v1::OverlayElement *up_button;
    Ogre::v1::OverlayElement *dn_button;
    Ogre::v1::OverlayElement *display;
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

    Ogre::v1::Overlay *dash_overlay;
    Ogre::v1::Overlay *needles_overlay;

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
    Ogre::v1::TextAreaOverlayElement* alt_value_textarea;

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

class OverlayWrapper : public ZeroedMemoryAllocator
{
public:

    OverlayWrapper();
    ~OverlayWrapper();

    struct LoadedOverlay
    {
        float orgScaleX;
        float orgScaleY;
        Ogre::v1::Overlay *o;
    };

    void ToggleDashboardOverlays(Actor *actor);

    void showDashboardOverlays(bool show, Actor *actor);

    void windowResized();
    void resizeOverlay(LoadedOverlay & overlay);

    bool mouseMoved(const OIS::MouseEvent& _arg);
    bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    float mTimeUntilNextToggle;

    void UpdatePressureOverlay(RoR::GfxActor* ga);
    void update(float dt);
    void UpdateLandVehicleHUD(RoR::GfxActor* ga);
    void UpdateAerialHUD(RoR::GfxActor* ga);
    void UpdateMarineHUD(Actor * vehicle);

    void ShowRacingOverlay();
    void HideRacingOverlay();
    void UpdateRacingGui(RoR::GfxScene* gs);

    Ogre::v1::Overlay *loadOverlay(Ogre::String name, bool autoResizeRation=true);

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
    void resizePanel(Ogre::v1::OverlayElement *oe);
    void reposPanel(Ogre::v1::OverlayElement *oe);
    void placeNeedle(Ogre::SceneNode *node, float x, float y, float len);
    void showPressureOverlay(bool show);

    Ogre::v1::OverlayElement *loadOverlayElement(Ogre::String name);

    Ogre::Window* win;

    bool m_dashboard_visible;

    // -------------------------------------------------------------
    // Overlays
    // -------------------------------------------------------------

    unsigned int  m_visible_overlays;

    Ogre::v1::Overlay *m_truck_pressure_overlay;
    Ogre::v1::Overlay *m_truck_pressure_needle_overlay;

    AeroDashOverlay m_aerial_dashboard;

    Ogre::v1::Overlay *m_marine_dashboard_overlay;
    Ogre::v1::Overlay *m_marine_dashboard_needles_overlay;

    Ogre::v1::Overlay *m_machine_dashboard_overlay;

    // Misc
    Ogre::v1::Overlay *m_racing_overlay;

    // -------------------------------------------------------------
    // Overlay elements
    // -------------------------------------------------------------

    // Truck
    Ogre::v1::OverlayElement* guiGear;      //!< truck
    Ogre::v1::OverlayElement* guiGear3D;    //!< truck

    // Marine overlay elements
    Ogre::v1::OverlayElement *bthro1;
    Ogre::v1::OverlayElement *bthro2;

    // Truck
    Ogre::v1::TextAreaOverlayElement* guiAuto[5];
    Ogre::v1::TextAreaOverlayElement* guiAuto3D[5];

    // Truck (racing overlay)
    Ogre::v1::TextAreaOverlayElement* laptime;
    Ogre::v1::TextAreaOverlayElement* bestlaptime;

    Ogre::v1::TextAreaOverlayElement* boat_depth_value_taoe; //!< Marine

    // truck
    Ogre::TextureUnitState *speedotexture; // Needed for dashboard prop
    Ogre::TextureUnitState *tachotexture;  // Needed for dashboard prop

    // Marine
    Ogre::TextureUnitState *boatspeedtexture;
    Ogre::TextureUnitState *boatsteertexture;

    // Truck
    Ogre::TextureUnitState *pressuretexture;

    // Marine: Written in init(), read-only in simulation.
    float thrtop;
    float thrheight;
    float throffset;

    std::vector<LoadedOverlay> m_loaded_overlays;
};

} // namespace RoR
