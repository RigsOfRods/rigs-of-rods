/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   OverlayWrapper.h
	@author Thomas Fischer
	@date   6th of May 2010
*/

#pragma once

#include "RoRPrerequisites.h"

#include <OgreTextAreaOverlayElement.h>
#include <OIS.h>

class OverlayWrapper : public ZeroedMemoryAllocator
{
	friend class RoRFrameListener;
	friend class RoR::Application;
	friend class RoR::MainThread;

public:

	struct LoadedOverlay
	{
		float orgScaleX;
		float orgScaleY;
		Ogre::Overlay *o;
	};

	void showDashboardOverlays(bool show, Beam *truck);
	void showDebugOverlay(int mode);
	void showPressureOverlay(bool show);
	void showEditorOverlay(bool show);

	void windowResized();
	void resizeOverlay(LoadedOverlay & overlay);

	int getDashBoardHeight();

	bool mouseMoved(const OIS::MouseEvent& _arg);
	bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
	bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
	float mTimeUntilNextToggle;

	void SetupDirectionArrow();

	void UpdateDirectionArrow(Beam* vehicle, Ogre::Vector3 const & point_to);

	void HideDirectionOverlay();

	void ShowDirectionOverlay(Ogre::String const & caption);

	void UpdatePressureTexture(float pressure);

	void UpdateLandVehicleHUD(Beam * vehicle, bool & flipflop);

	void UpdateAerialHUD(Beam * vehicle);

	void UpdateMarineHUD(Beam * vehicle);

protected:

	OverlayWrapper();
	~OverlayWrapper();

	int init();
	void update(float dt);
	void resizePanel(Ogre::OverlayElement *oe);
	void reposPanel(Ogre::OverlayElement *oe);
	void placeNeedle(Ogre::SceneNode *node, float x, float y, float len);
	void updateStats(bool detailed=false);

	Ogre::Overlay *loadOverlay(Ogre::String name, bool autoResizeRation=true);
	Ogre::OverlayElement *loadOverlayElement(Ogre::String name);

	Ogre::RenderWindow* win;
	TruckHUD *truckhud;

	// Misc
	Ogre::Overlay *directionOverlay;        //!< truck (racing)
	Ogre::Overlay *mDebugOverlay;
	Ogre::Overlay *mTimingDebugOverlay;
	Ogre::Overlay *dashboardOverlay;        //!< truck
	Ogre::Overlay *machinedashboardOverlay; 
	Ogre::Overlay *airdashboardOverlay;     //!< aerial
	Ogre::Overlay *boatdashboardOverlay;    //!< marine
	Ogre::Overlay *needlesOverlay;          //!< truck
	Ogre::Overlay *airneedlesOverlay;       //!< aerial
	Ogre::Overlay *boatneedlesOverlay;      //!< marine
	Ogre::Overlay *needlesMaskOverlay;      //!< truck
	Ogre::Overlay *pressureOverlay;         //!< truck
	Ogre::Overlay *pressureNeedleOverlay;   //!< truck
	Ogre::Overlay *editorOverlay;           //!< UNUSED
	Ogre::Overlay *truckeditorOverlay;      //!< UNUSED
	Ogre::Overlay *mouseOverlay;            //!< UNUSED (aerial)
	Ogre::Overlay *racing;                  //!< truck (racing)

	// Truck
	Ogre::OverlayElement* guiGear;      //!< truck
	Ogre::OverlayElement* guiGear3D;    //!< truck
	Ogre::OverlayElement* guiRoll;      //!< truck
	Ogre::OverlayElement* guipedclutch; //!< truck
	Ogre::OverlayElement* guipedbrake;  //!< truck
	Ogre::OverlayElement* guipedacc;    //!< truck
	Ogre::OverlayElement* mouseElement; //!< UNUSED
	Ogre::OverlayElement *pbrakeo;      //!< truck
	Ogre::OverlayElement *tcontrolo;    //!< truck
	Ogre::OverlayElement *antilocko;    //!< truck
	Ogre::OverlayElement *lockedo;      //!< truck
	Ogre::OverlayElement *securedo;     //!< truck
	Ogre::OverlayElement *lopresso;     //!< truck
	Ogre::OverlayElement *clutcho;      //!< truck
	Ogre::OverlayElement *lightso;      //!< truck
	Ogre::OverlayElement *batto;        //!< truck
	Ogre::OverlayElement *igno;         //!< truck

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
	Ogre::OverlayElement *editor_pos;
	Ogre::OverlayElement *editor_angles;
	Ogre::OverlayElement *editor_object;

	// Truck
	Ogre::TextAreaOverlayElement* guiAuto[5];
	Ogre::TextAreaOverlayElement* guiAuto3D[5];

	// Truck (racing)
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
	Ogre::TextureUnitState *speedotexture;
	Ogre::TextureUnitState *tachotexture;
	Ogre::TextureUnitState *rolltexture;
	Ogre::TextureUnitState *pitchtexture;
	Ogre::TextureUnitState *rollcortexture;
	Ogre::TextureUnitState *turbotexture;

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

	// Truck racing overlay
	Ogre::SceneNode* m_direction_arrow_node;

protected:

	std::vector<LoadedOverlay> m_loaded_overlays;
};
