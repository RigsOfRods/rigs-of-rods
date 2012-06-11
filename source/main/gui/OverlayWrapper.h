/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 6th of May 2010
#ifndef __OverlayWrapper_H_
#define __OverlayWrapper_H_

#include "RoRPrerequisites.h"

#include "Singleton.h"

#include "OgreTextAreaOverlayElement.h"
#include <OIS.h>

struct loadedOverlay_t
{
	float orgScaleX;
	float orgScaleY;
	Ogre::Overlay *o;
};

class OverlayWrapper : public RoRSingletonNoCreation<OverlayWrapper>, public ZeroedMemoryAllocator
{
	friend class RoRFrameListener;

public:

	OverlayWrapper();
	~OverlayWrapper();

	// functions
	void showDashboardOverlays(bool show, Beam *truck);
	void showDebugOverlay(int mode);
	void showPressureOverlay(bool show);
	void showEditorOverlay(bool show);

	void windowResized();
	void resizeOverlay(struct loadedOverlay_t);

	int getDashBoardHeight();

	bool mouseMoved(const OIS::MouseEvent& _arg);
	bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
	bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
	float mTimeUntilNextToggle;

protected:

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

	//members
	Ogre::Overlay *directionOverlay;
	Ogre::Overlay *mDebugOverlay;
	Ogre::Overlay *mTimingDebugOverlay;
	Ogre::Overlay *dashboardOverlay;
	Ogre::Overlay *machinedashboardOverlay;
	Ogre::Overlay *airdashboardOverlay;
	Ogre::Overlay *boatdashboardOverlay;
	Ogre::Overlay *needlesOverlay;
	Ogre::Overlay *airneedlesOverlay;
	Ogre::Overlay *boatneedlesOverlay;
	Ogre::Overlay *needlesMaskOverlay;
	Ogre::Overlay *pressureOverlay;
	Ogre::Overlay *pressureNeedleOverlay;
	Ogre::Overlay *editorOverlay;
	Ogre::Overlay *truckeditorOverlay;
	Ogre::Overlay *mouseOverlay;
	Ogre::Overlay *racing;

	Ogre::OverlayElement* guiGear;
	Ogre::OverlayElement* guiGear3D;
	Ogre::OverlayElement* guiRoll;
	Ogre::OverlayElement* guipedclutch;
	Ogre::OverlayElement* guipedbrake;
	Ogre::OverlayElement* guipedacc;
	Ogre::OverlayElement* mouseElement;
	Ogre::OverlayElement *pbrakeo;
	Ogre::OverlayElement *tcontrolo;
	Ogre::OverlayElement *antilocko;
	Ogre::OverlayElement *lockedo;
	Ogre::OverlayElement *securedo;
	Ogre::OverlayElement *lopresso;
	Ogre::OverlayElement *clutcho;
	Ogre::OverlayElement *lightso;
	Ogre::OverlayElement *batto;
	Ogre::OverlayElement *igno;
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
	Ogre::OverlayElement *bthro1;
	Ogre::OverlayElement *bthro2;
	Ogre::OverlayElement *editor_pos;
	Ogre::OverlayElement *editor_angles;
	Ogre::OverlayElement *editor_object;

	Ogre::TextAreaOverlayElement* guiAuto[5];
	Ogre::TextAreaOverlayElement* guiAuto3D[5];
	Ogre::TextAreaOverlayElement* laptimemin;
	Ogre::TextAreaOverlayElement* laptimes;
	Ogre::TextAreaOverlayElement* laptimems;
	Ogre::TextAreaOverlayElement* lasttime;
	Ogre::TextAreaOverlayElement* directionArrowText;
	Ogre::TextAreaOverlayElement* directionArrowDistance;
	Ogre::TextAreaOverlayElement* alt_value_taoe;
	Ogre::TextAreaOverlayElement* boat_depth_value_taoe;

	Ogre::TextureUnitState *adibugstexture;
	Ogre::TextureUnitState *aditapetexture;
	Ogre::TextureUnitState *hsirosetexture;
	Ogre::TextureUnitState *hsibugtexture;
	Ogre::TextureUnitState *hsivtexture;
	Ogre::TextureUnitState *hsihtexture;
	Ogre::TextureUnitState *speedotexture;
	Ogre::TextureUnitState *tachotexture;
	Ogre::TextureUnitState *rolltexture;
	Ogre::TextureUnitState *pitchtexture;
	Ogre::TextureUnitState *rollcortexture;
	Ogre::TextureUnitState *turbotexture;
	Ogre::TextureUnitState *airspeedtexture;
	Ogre::TextureUnitState *altimetertexture;
	Ogre::TextureUnitState *vvitexture;
	Ogre::TextureUnitState *aoatexture;
	Ogre::TextureUnitState *boatspeedtexture;
	Ogre::TextureUnitState *boatsteertexture;
	Ogre::TextureUnitState *pressuretexture;
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

	float thrtop;
	float thrheight;
	float throffset;

protected:

	std::vector<struct loadedOverlay_t> overlays;
};

#endif // __OverlayWrapper_H_
