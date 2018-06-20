/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2014 Petr Ohlidal

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

#include "RoRPrerequisites.h"

#include <Overlay/OgreTextAreaOverlayElement.h>
#include <OIS.h>

class OverlayWrapper : public ZeroedMemoryAllocator
{
    friend class SimController;
    friend class RoR::MainMenu;

public:

    OverlayWrapper();
    ~OverlayWrapper();

    struct LoadedOverlay
    {
        float orgScaleX;
        float orgScaleY;
        Ogre::Overlay *o;
    };

    void showDashboardOverlays(bool show, Actor *actor);
    void showDebugOverlay(int mode);
    void showPressureOverlay(bool show);

    void windowResized();
    void resizeOverlay(LoadedOverlay & overlay);

    bool mouseMoved(const OIS::MouseEvent& _arg);
    bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    float mTimeUntilNextToggle;

    void SetupDirectionArrow();
    void UpdateDirectionArrow(Actor* vehicle, Ogre::Vector3 const & point_to);
    void HideDirectionOverlay();
    void ShowDirectionOverlay(Ogre::String const & caption);

    void UpdatePressureTexture(float pressure);

    void UpdateLandVehicleHUD(Actor * vehicle);
    void UpdateAerialHUD(Actor * vehicle);
    void UpdateMarineHUD(Actor * vehicle);

    void ShowRacingOverlay();
    void HideRacingOverlay();

    /// Hides all overlays, but doesn't change visibility flags (for further restoring).
    void TemporarilyHideAllOverlays(Actor *current_vehicle);

    /// Shows all overlays flagged as "visible".
    void RestoreOverlaysVisibility(Actor *current_vehicle);

protected:

    /// RoR needs to temporarily hide all overlays when player enters editor. 
    /// However, OGRE only provides per-overlay show() and hide() functionality.
    /// Thus, an external state must be kept to restore overlays after exiting the editor.
    struct VisibleOverlays
    {
        static const int DIRECTION_ARROW              = BITMASK(1);
        static const int DEBUG_FPS_MEMORY             = BITMASK(2);
        static const int DEBUG_BEAM_TIMING            = BITMASK(3);
        static const int RACING                       = BITMASK(4);
        static const int TRUCK_TIRE_PRESSURE_OVERLAY  = BITMASK(5);
    };

    int init();
    void update(float dt);
    void resizePanel(Ogre::OverlayElement *oe);
    void reposPanel(Ogre::OverlayElement *oe);
    void placeNeedle(Ogre::SceneNode *node, float x, float y, float len);
    void updateStats(bool detailed=false);

    Ogre::Overlay *loadOverlay(Ogre::String name, bool autoResizeRation=true);
    Ogre::OverlayElement *loadOverlayElement(Ogre::String name);

    Ogre::RenderWindow* win;

    // -------------------------------------------------------------
    // Overlays
    // -------------------------------------------------------------

    unsigned int  m_visible_overlays;

    Ogre::Overlay *m_truck_pressure_overlay;
    Ogre::Overlay *m_truck_pressure_needle_overlay;

    Ogre::Overlay *m_aerial_dashboard_overlay;
    Ogre::Overlay *m_aerial_dashboard_needles_overlay;

    Ogre::Overlay *m_marine_dashboard_overlay;
    Ogre::Overlay *m_marine_dashboard_needles_overlay;

    Ogre::Overlay *m_machine_dashboard_overlay;

    // Misc
    Ogre::Overlay *m_direction_arrow_overlay;
    Ogre::Overlay *m_debug_fps_memory_overlay;
    Ogre::Overlay *m_debug_beam_timing_overlay;	
    Ogre::Overlay *m_racing_overlay;

    // -------------------------------------------------------------
    // Overlay elements
    // -------------------------------------------------------------

    // Truck
    Ogre::OverlayElement* guiGear;      //!< truck
    Ogre::OverlayElement* guiGear3D;    //!< truck

    // Aerial overlay elements
    Ogre::OverlayElement *thro1;
    Ogre::OverlayElement *thro2;
    Ogre::OverlayElement *thro3;
    Ogre::OverlayElement *thro4;
    Ogre::OverlayElement *engfireo1;
    Ogre::OverlayElement *engfireo2;
    Ogre::OverlayElement *engfireo3;
    Ogre::OverlayElement *engfireo4;
    Ogre::OverlayElement *engstarto1;
    Ogre::OverlayElement *engstarto2;
    Ogre::OverlayElement *engstarto3;
    Ogre::OverlayElement *engstarto4;

    // Marine overlay elements
    Ogre::OverlayElement *bthro1;
    Ogre::OverlayElement *bthro2;

    // Truck
    Ogre::TextAreaOverlayElement* guiAuto[5];
    Ogre::TextAreaOverlayElement* guiAuto3D[5];

    // Truck (m_racing_overlay)
    Ogre::TextAreaOverlayElement* laptimemin;
    Ogre::TextAreaOverlayElement* laptimes;
    Ogre::TextAreaOverlayElement* laptimems;
    Ogre::TextAreaOverlayElement* lasttime;
    Ogre::TextAreaOverlayElement* directionArrowText;
    Ogre::TextAreaOverlayElement* directionArrowDistance;

    Ogre::TextAreaOverlayElement* alt_value_taoe; //!!< Aerial

    Ogre::TextAreaOverlayElement* boat_depth_value_taoe; //!< Marine

    // Aerial
    Ogre::TextureUnitState *adibugstexture;
    Ogre::TextureUnitState *aditapetexture;
    Ogre::TextureUnitState *hsirosetexture;
    Ogre::TextureUnitState *hsibugtexture;
    Ogre::TextureUnitState *hsivtexture;
    Ogre::TextureUnitState *hsihtexture;

    // truck
    Ogre::TextureUnitState *speedotexture; // Needed for dashboard prop
    Ogre::TextureUnitState *tachotexture;  // Needed for dashboard prop

    // Aerial
    Ogre::TextureUnitState *airspeedtexture;
    Ogre::TextureUnitState *altimetertexture;
    Ogre::TextureUnitState *vvitexture;
    Ogre::TextureUnitState *aoatexture;

    // Marine
    Ogre::TextureUnitState *boatspeedtexture;
    Ogre::TextureUnitState *boatsteertexture;

    // Truck
    Ogre::TextureUnitState *pressuretexture;
    
    // Aerial
    Ogre::TextureUnitState *airrpm1texture;
    Ogre::TextureUnitState *airrpm2texture;
    Ogre::TextureUnitState *airrpm3texture;
    Ogre::TextureUnitState *airrpm4texture;
    Ogre::TextureUnitState *airpitch1texture;
    Ogre::TextureUnitState *airpitch2texture;
    Ogre::TextureUnitState *airpitch3texture;
    Ogre::TextureUnitState *airpitch4texture;
    Ogre::TextureUnitState *airtorque1texture;
    Ogre::TextureUnitState *airtorque2texture;
    Ogre::TextureUnitState *airtorque3texture;
    Ogre::TextureUnitState *airtorque4texture;

    // Aerial + Marine: Written in init(), read-only in simulation.
    float thrtop;
    float thrheight;
    float throffset;

    // Truck m_racing_overlay overlay
    Ogre::SceneNode* m_direction_arrow_node;

    std::vector<LoadedOverlay> m_loaded_overlays;
};
