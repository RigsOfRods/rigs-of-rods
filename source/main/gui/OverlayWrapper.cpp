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
#include "OverlayWrapper.h"

#include "AeroEngine.h"
#include "AutoPilot.h"
#include "BeamFactory.h"
#include "DashBoardManager.h"
#include "ErrorUtils.h"
#include "Language.h"
#include "OgreFontManager.h"
#include "RoRVersion.h"
#include "TruckHUD.h"
#include "Utils.h"

using namespace Ogre;

OverlayWrapper::OverlayWrapper()
{
	setSingleton(this);
	win = gEnv->renderWindow;
	init();
	mTimeUntilNextToggle=0;

}

OverlayWrapper::~OverlayWrapper()
{
}


void OverlayWrapper::resizePanel(OverlayElement *oe)
{
	oe->setHeight(oe->getHeight()*(Real)win->getWidth()/(Real)win->getHeight());
	oe->setTop(oe->getTop()*(Real)win->getWidth()/(Real)win->getHeight());
}

void OverlayWrapper::reposPanel(OverlayElement *oe)
{
	oe->setTop(oe->getTop()*(Real)win->getWidth()/(Real)win->getHeight());
}

void OverlayWrapper::placeNeedle(SceneNode *node, float x, float y, float len)
{/**(Real)win->getHeight()/(Real)win->getWidth()*/
	node->setPosition((x-640.0)/444.0, (512-y)/444.0, -2.0);
	node->setScale(0.0025, 0.007*len, 0.007);
}

Overlay *OverlayWrapper::loadOverlay(String name, bool autoResizeRation)
{
	Overlay *o = OverlayManager::getSingleton().getByName(name);
	if (!o) return NULL;

	if (autoResizeRation)
	{
		struct loadedOverlay_t lo;
		lo.o = o;
		lo.orgScaleX = o->getScaleX();
		lo.orgScaleY = o->getScaleY();

		overlays.push_back(lo);
		resizeOverlay(lo);
	}
	return o;
}

void OverlayWrapper::resizeOverlay(struct loadedOverlay_t lo)
{
	// enforce 4:3 for overlays
	float w = win->getWidth();
	float h = win->getHeight();
	float s = (4.0f/3.0f) / (w/h);

	// window is higher than wide
	if (s > 1)
		s = (3.0f/4.0f) / (h/w);

	// originals
	lo.o->setScale(lo.orgScaleX, lo.orgScaleY);
	lo.o->setScroll(0, 0);

	// now the new values
	lo.o->setScale(s, s);
	lo.o->scroll(1 - s, s - 1);
}

void OverlayWrapper::windowResized()
{
	for (std::vector<struct loadedOverlay_t>::iterator it = overlays.begin(); it != overlays.end(); it++)
	{
		resizeOverlay(*it);
	}
}

OverlayElement *OverlayWrapper::loadOverlayElement(String name)
{
	return OverlayManager::getSingleton().getOverlayElement(name);
}

int OverlayWrapper::init()
{

	directionOverlay = loadOverlay("tracks/DirectionArrow", false);
	try
	{
		directionArrowText = (TextAreaOverlayElement*)loadOverlayElement("tracks/DirectionArrow/Text");
	} catch(...)
	{
		String url = "http://wiki.rigsofrods.com/index.php?title=Error_Resources_Not_Found";
		showOgreWebError("Resources not found!", "please ensure that your installation is complete and the resources are installed properly. If this error persists please re-install RoR.", url);
	}
	directionArrowDistance = (TextAreaOverlayElement*)loadOverlayElement("tracks/DirectionArrow/Distance");

	// openGL fix
	directionOverlay->show();
	directionOverlay->hide();

	mouseOverlay = loadOverlay("tracks/MouseOverlay", false);
#ifdef HAS_EDITOR
	truckeditorOverlay = loadOverlay("tracks/TruckEditorOverlay");
#endif
	mDebugOverlay = loadOverlay("Core/DebugOverlay", false);
	mTimingDebugOverlay = loadOverlay("tracks/DebugBeamTiming", false);
	mTimingDebugOverlay->hide();

	OverlayElement *vere = loadOverlayElement("Core/RoRVersionString");
	if (vere) vere->setCaption("Rigs of Rods version " + String(ROR_VERSION_STRING));
	


	machinedashboardOverlay = loadOverlay("tracks/MachineDashboardOverlay");
	airdashboardOverlay = loadOverlay("tracks/AirDashboardOverlay", false);
	airneedlesOverlay = loadOverlay("tracks/AirNeedlesOverlay", false);
	boatdashboardOverlay = loadOverlay("tracks/BoatDashboardOverlay");
	boatneedlesOverlay = loadOverlay("tracks/BoatNeedlesOverlay");
	dashboardOverlay = loadOverlay("tracks/DashboardOverlay");
	needlesOverlay = loadOverlay("tracks/NeedlesOverlay");
	needlesMaskOverlay = loadOverlay("tracks/NeedlesMaskOverlay");


	//adjust dashboard size for screen ratio
	resizePanel(loadOverlayElement("tracks/pressureo"));
	resizePanel(loadOverlayElement("tracks/pressureneedle"));
	MaterialPtr m = MaterialManager::getSingleton().getByName("tracks/pressureneedle_mat");
	if (!m.isNull())
		pressuretexture=m->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/speedo"));
	resizePanel(loadOverlayElement("tracks/tacho"));
	resizePanel(loadOverlayElement("tracks/anglo"));
	resizePanel(loadOverlayElement("tracks/instructions"));
	resizePanel(loadOverlayElement("tracks/machineinstructions"));
	resizePanel(loadOverlayElement("tracks/dashbar"));
	resizePanel(loadOverlayElement("tracks/dashfiller")	);
	resizePanel(loadOverlayElement("tracks/helppanel"));
	resizePanel(loadOverlayElement("tracks/machinehelppanel"));

	resizePanel(igno=loadOverlayElement("tracks/ign"));
	resizePanel(batto=loadOverlayElement("tracks/batt"));
	resizePanel(pbrakeo=loadOverlayElement("tracks/pbrake"));
	resizePanel(tcontrolo=loadOverlayElement("tracks/tcontrol"));
	resizePanel(antilocko=loadOverlayElement("tracks/antilock"));
	resizePanel(lockedo=loadOverlayElement("tracks/locked"));
	resizePanel(securedo=loadOverlayElement("tracks/secured"));
	resizePanel(lopresso=loadOverlayElement("tracks/lopress"));
	resizePanel(clutcho=loadOverlayElement("tracks/clutch"));
	resizePanel(lightso=loadOverlayElement("tracks/lights"));

	mouseElement = loadOverlayElement("mouse/pointer");

	resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/machinedashbar"));
	resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/machinedashfiller"));

	resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/airdashbar"));
	resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/airdashfiller"));
	
	OverlayElement* tempoe;
	resizePanel(tempoe=OverlayManager::getSingleton().getOverlayElement("tracks/thrusttrack1"));

	resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/thrusttrack2"));
	resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/thrusttrack3"));
	resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/thrusttrack4"));

	resizePanel(thro1=loadOverlayElement("tracks/thrust1"));
	resizePanel(thro2=loadOverlayElement("tracks/thrust2"));
	resizePanel(thro3=loadOverlayElement("tracks/thrust3"));
	resizePanel(thro4=loadOverlayElement("tracks/thrust4"));

	thrtop = 1.0f + tempoe->getTop() + thro1->getHeight() * 0.5f;
	thrheight = tempoe->getHeight() - thro1->getHeight() * 2.0f;
	throffset = thro1->getHeight() * 0.5f;

	engfireo1=loadOverlayElement("tracks/engfire1");
	engfireo2=loadOverlayElement("tracks/engfire2");
	engfireo3=loadOverlayElement("tracks/engfire3");
	engfireo4=loadOverlayElement("tracks/engfire4");
	engstarto1=loadOverlayElement("tracks/engstart1");
	engstarto2=loadOverlayElement("tracks/engstart2");
	engstarto3=loadOverlayElement("tracks/engstart3");
	engstarto4=loadOverlayElement("tracks/engstart4");
	resizePanel(loadOverlayElement("tracks/airrpm1"));
	resizePanel(loadOverlayElement("tracks/airrpm2"));
	resizePanel(loadOverlayElement("tracks/airrpm3"));
	resizePanel(loadOverlayElement("tracks/airrpm4"));
	resizePanel(loadOverlayElement("tracks/airpitch1"));
	resizePanel(loadOverlayElement("tracks/airpitch2"));
	resizePanel(loadOverlayElement("tracks/airpitch3"));
	resizePanel(loadOverlayElement("tracks/airpitch4"));
	resizePanel(loadOverlayElement("tracks/airtorque1"));
	resizePanel(loadOverlayElement("tracks/airtorque2"));
	resizePanel(loadOverlayElement("tracks/airtorque3"));
	resizePanel(loadOverlayElement("tracks/airtorque4"));
	resizePanel(loadOverlayElement("tracks/airspeed"));
	resizePanel(loadOverlayElement("tracks/vvi"));
	resizePanel(loadOverlayElement("tracks/altimeter"));
	resizePanel(loadOverlayElement("tracks/altimeter_val"));
	alt_value_taoe=(TextAreaOverlayElement*)loadOverlayElement("tracks/altimeter_val");
	boat_depth_value_taoe=(TextAreaOverlayElement*)loadOverlayElement("tracks/boatdepthmeter_val");
	resizePanel(loadOverlayElement("tracks/adi-tape"));
	resizePanel(loadOverlayElement("tracks/adi"));
	resizePanel(loadOverlayElement("tracks/adi-bugs"));
	adibugstexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/adi-bugs")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	aditapetexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/adi-tape")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/aoa"));
	resizePanel(loadOverlayElement("tracks/hsi"));
	resizePanel(loadOverlayElement("tracks/hsi-rose"));
	resizePanel(loadOverlayElement("tracks/hsi-bug"));
	resizePanel(loadOverlayElement("tracks/hsi-v"));
	resizePanel(loadOverlayElement("tracks/hsi-h"));
	hsirosetexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/hsi-rose")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	hsibugtexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/hsi-bug")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	hsivtexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/hsi-v")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	hsihtexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/hsi-h")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	//autopilot
	reposPanel(loadOverlayElement("tracks/ap_hdg_pack"));
	reposPanel(loadOverlayElement("tracks/ap_wlv_but"));
	reposPanel(loadOverlayElement("tracks/ap_nav_but"));
	reposPanel(loadOverlayElement("tracks/ap_alt_pack"));
	reposPanel(loadOverlayElement("tracks/ap_vs_pack"));
	reposPanel(loadOverlayElement("tracks/ap_ias_pack"));
	reposPanel(loadOverlayElement("tracks/ap_gpws_but"));
	reposPanel(loadOverlayElement("tracks/ap_brks_but"));

	//boat
	resizePanel(loadOverlayElement("tracks/boatdashbar"));
	resizePanel(loadOverlayElement("tracks/boatdashfiller"));
	resizePanel(loadOverlayElement("tracks/boatthrusttrack1"));
	resizePanel(loadOverlayElement("tracks/boatthrusttrack2"));

	//resizePanel(boatmapo=loadOverlayElement("tracks/boatmap"));
	//resizePanel(boatmapdot=loadOverlayElement("tracks/boatreddot"));

	resizePanel(bthro1=loadOverlayElement("tracks/boatthrust1"));
	resizePanel(bthro2=loadOverlayElement("tracks/boatthrust2"));

	resizePanel(loadOverlayElement("tracks/boatspeed"));
	resizePanel(loadOverlayElement("tracks/boatsteer"));
	resizePanel(loadOverlayElement("tracks/boatspeedneedle"));
	resizePanel(loadOverlayElement("tracks/boatsteer/fg"));
	boatspeedtexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/boatspeedneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	boatsteertexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/boatsteer/fg_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);


	//adjust dashboard mask
	//pitch mask
	OverlayElement *pmask=loadOverlayElement("tracks/pitchmask");
	pmask->setHeight(pmask->getHeight()*(Real)win->getWidth()/(Real)win->getHeight());
	pmask->setTop(pmask->getTop()*(Real)win->getWidth()/(Real)win->getHeight());
	//roll mask
	OverlayElement *rmask=loadOverlayElement("tracks/rollmask");
	rmask->setHeight(rmask->getHeight()*(Real)win->getWidth()/(Real)win->getHeight());
	rmask->setTop(rmask->getTop()*(Real)win->getWidth()/(Real)win->getHeight());

	//prepare needles
	resizePanel(loadOverlayElement("tracks/speedoneedle"));
	speedotexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/speedoneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/tachoneedle"));
	tachotexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/tachoneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/rollneedle"));
	rolltexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/rollneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/pitchneedle"));
	pitchtexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/pitchneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/rollcorneedle"));
	rollcortexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/rollcorneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/turboneedle"));
	turbotexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/turboneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/airspeedneedle"));
	airspeedtexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airspeedneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/altimeterneedle"));
	altimetertexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/altimeterneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/vvineedle"));
	vvitexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/vvineedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/aoaneedle"));
	aoatexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/aoaneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);


	resizePanel(loadOverlayElement("tracks/airrpm1needle"));
	airrpm1texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airrpm1needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/airrpm2needle"));
	airrpm2texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airrpm2needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/airrpm3needle"));
	airrpm3texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airrpm3needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/airrpm4needle"));
	airrpm4texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airrpm4needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/airpitch1needle"));
	airpitch1texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airpitch1needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/airpitch2needle"));
	airpitch2texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airpitch2needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/airpitch3needle"));
	airpitch3texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airpitch3needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/airpitch4needle"));
	airpitch4texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airpitch4needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/airtorque1needle"));
	airtorque1texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airtorque1needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/airtorque2needle"));
	airtorque2texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airtorque2needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/airtorque3needle"));
	airtorque3texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airtorque3needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/airtorque4needle"));
	airtorque4texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airtorque4needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	//		speedotexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/speedoneedle")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	//        Entity *ecs=gEnv->ogreSceneManager->createEntity("speed_needle", "needle.mesh");
	//        speed_node=gEnv->ogreSceneManager->getRootSceneNode()->createChildSceneNode();
	//        speed_node->attachObject(ecs);
	//		needlesOverlay->add3D(speed_node);
	//		placeNeedle(win, speed_node, 1189, 936, 1.0);
	//tacho
	//        Entity *ect=gEnv->ogreSceneManager->createEntity("tacho_needle", "needle.mesh");
	//        tach_node=gEnv->ogreSceneManager->getRootSceneNode()->createChildSceneNode();
	//        tach_node->attachObject(ect);
	//		needlesOverlay->add3D(tach_node);
	//		placeNeedle(win, tach_node, 1011, 935, 1.0);
	//		//pitch
	//        Entity *ecp=gEnv->ogreSceneManager->createEntity("pitch_needle", "needle.mesh");
	//		ecp->setMaterialName("tracks/whiteneedle");
	//        pitch_node=gEnv->ogreSceneManager->getRootSceneNode()->createChildSceneNode();
	//        pitch_node->attachObject(ecp);
	//		needlesOverlay->add3D(pitch_node);
	//		placeNeedle(win, pitch_node, 876, 1014, 1.0);
	//roll
	//        Entity *ecr=gEnv->ogreSceneManager->createEntity("roll_needle", "needle.mesh");
	//		ecr->setMaterialName("tracks/whiteneedle");
	//        roll_node=gEnv->ogreSceneManager->getRootSceneNode()->createChildSceneNode();
	//        roll_node->attachObject(ecr);
	//		needlesOverlay->add3D(roll_node);
	//		placeNeedle(win, roll_node, 876, 924, 1.0);

	//rollcorr
	//		Entity *ecrc=gEnv->ogreSceneManager->createEntity("rollcorr_needle", "needle.mesh");
	//		ecrc->setMaterialName("tracks/stabneedle");
	//		rollcorr_node=gEnv->ogreSceneManager->getRootSceneNode()->createChildSceneNode();
	//		rollcorr_node->attachObject(ecrc);
	//		needlesOverlay->add3D(rollcorr_node);
	//		placeNeedle(win, rollcorr_node, 876, 924, 0.8);


	guiGear=loadOverlayElement("tracks/Gear");
	guiGear3D=loadOverlayElement("tracks/3DGear");
	guiRoll=loadOverlayElement("tracks/rollmask");

	guiAuto[0]=(TextAreaOverlayElement*)loadOverlayElement("tracks/AGearR");
	guiAuto[1]=(TextAreaOverlayElement*)loadOverlayElement("tracks/AGearN");
	guiAuto[2]=(TextAreaOverlayElement*)loadOverlayElement("tracks/AGearD");
	guiAuto[3]=(TextAreaOverlayElement*)loadOverlayElement("tracks/AGear2");
	guiAuto[4]=(TextAreaOverlayElement*)loadOverlayElement("tracks/AGear1");

	guiAuto3D[0]=(TextAreaOverlayElement*)loadOverlayElement("tracks/3DAGearR");
	guiAuto3D[1]=(TextAreaOverlayElement*)loadOverlayElement("tracks/3DAGearN");
	guiAuto3D[2]=(TextAreaOverlayElement*)loadOverlayElement("tracks/3DAGearD");
	guiAuto3D[3]=(TextAreaOverlayElement*)loadOverlayElement("tracks/3DAGear2");
	guiAuto3D[4]=(TextAreaOverlayElement*)loadOverlayElement("tracks/3DAGear1");

	guipedclutch=loadOverlayElement("tracks/pedalclutch");
	guipedbrake=loadOverlayElement("tracks/pedalbrake");
	guipedacc=loadOverlayElement("tracks/pedalacc");

	pressureOverlay = loadOverlay("tracks/PressureOverlay");
	pressureNeedleOverlay = loadOverlay("tracks/PressureNeedleOverlay");
	editorOverlay = loadOverlay("tracks/EditorOverlay");

	racing = loadOverlay("tracks/Racing", false);
	laptimemin = (TextAreaOverlayElement*)loadOverlayElement("tracks/LapTimemin");
	laptimes = (TextAreaOverlayElement*)loadOverlayElement("tracks/LapTimes");
	laptimems = (TextAreaOverlayElement*)loadOverlayElement("tracks/LapTimems");
	lasttime = (TextAreaOverlayElement*)loadOverlayElement("tracks/LastTime");
	lasttime->setCaption("");

	// openGL fix
	racing->show();
	racing->hide();
	
	editor_pos = loadOverlayElement("tracks/EditorPosition");
	editor_angles = loadOverlayElement("tracks/EditorAngles");
	editor_object = loadOverlayElement("tracks/EditorObject");
	

	truckhud = new TruckHUD();
	truckhud->show(false);


	return 0;
}

void OverlayWrapper::update(float dt)
{
	if (mTimeUntilNextToggle > 0)
		mTimeUntilNextToggle-=dt;
}

void OverlayWrapper::showDebugOverlay(int mode)
{
	if (!mDebugOverlay || !mTimingDebugOverlay) return;
	if (mode > 0)
	{
		mDebugOverlay->show();
		if (mode > 1)
			mTimingDebugOverlay->show();
		else
			mTimingDebugOverlay->hide();
	}
	else
	{
		mDebugOverlay->hide();
		mTimingDebugOverlay->hide();
	}
}


void OverlayWrapper::showPressureOverlay(bool show)
{
	if (pressureOverlay)
	{
		if (show)
		{
			pressureOverlay->show();
			pressureNeedleOverlay->show();
		}
		else
		{
			pressureOverlay->hide();
			pressureNeedleOverlay->hide();
		}
	}
}

void OverlayWrapper::showEditorOverlay(bool show)
{
	if (editorOverlay)
	{
		if (show)
		{
			editorOverlay->show();
		}
		else
		{
			editorOverlay->hide();
		}
	}
}

void OverlayWrapper::showDashboardOverlays(bool show, Beam *truck)
{
	if (!needlesOverlay || !dashboardOverlay) return;
	int mode = -1;
	if (truck)
		mode = truck->driveable;

	// check if we use the new style dashboards
	if (truck && truck->dash && truck->dash->wasLoaded())
	{
		truck->dash->setVisible(show);
		return;
	}
	
	if (show)
	{
		if (mode==TRUCK)
		{
			needlesMaskOverlay->show();
			needlesOverlay->show();
			dashboardOverlay->show();
		};
		if (mode==AIRPLANE)
		{
			airneedlesOverlay->show();
			airdashboardOverlay->show();
			//mouseOverlay->show();
		};
		if (mode==BOAT)
		{
			boatneedlesOverlay->show();
			boatdashboardOverlay->show();
		};
		if (mode==BOAT)
		{
			boatneedlesOverlay->show();
			boatdashboardOverlay->show();
		};
		if (mode==MACHINE)
		{
			machinedashboardOverlay->show();
		};
	}
	else
	{
		machinedashboardOverlay->hide();
		needlesMaskOverlay->hide();
		needlesOverlay->hide();
		dashboardOverlay->hide();
		//for the airplane
		airneedlesOverlay->hide();
		airdashboardOverlay->hide();
		//mouseOverlay->hide();
		boatneedlesOverlay->hide();
		boatdashboardOverlay->hide();
	}
}

void OverlayWrapper::updateStats(bool detailed)
{
	static UTFString currFps  = _L("Current FPS: ");
	static UTFString avgFps   = _L("Average FPS: ");
	static UTFString bestFps  = _L("Best FPS: ");
	static UTFString worstFps = _L("Worst FPS: ");
	static UTFString tris     = _L("Triangle Count: ");
	const RenderTarget::FrameStats& stats = win->getStatistics();

	// update stats when necessary
	try
	{
		OverlayElement* guiAvg   = OverlayManager::getSingleton().getOverlayElement("Core/AverageFps");
		OverlayElement* guiCurr  = OverlayManager::getSingleton().getOverlayElement("Core/CurrFps");
		OverlayElement* guiBest  = OverlayManager::getSingleton().getOverlayElement("Core/BestFps");
		OverlayElement* guiWorst = OverlayManager::getSingleton().getOverlayElement("Core/WorstFps");


		guiAvg->setCaption(avgFps + TOUTFSTRING(stats.avgFPS));
		guiCurr->setCaption(currFps + TOUTFSTRING(stats.lastFPS));
		guiBest->setCaption(bestFps + TOUTFSTRING(stats.bestFPS) + U(" ") + TOUTFSTRING(stats.bestFrameTime) + U(" ms"));
		guiWorst->setCaption(worstFps + TOUTFSTRING(stats.worstFPS) + U(" ") + TOUTFSTRING(stats.worstFrameTime) + U(" ms"));

		OverlayElement* guiTris = OverlayManager::getSingleton().getOverlayElement("Core/NumTris");
		UTFString triss = tris + TOUTFSTRING(stats.triangleCount);
		if (stats.triangleCount > 1000000)
			triss = tris + TOUTFSTRING(stats.triangleCount/1000000.0f) + U(" M");
		else if (stats.triangleCount > 1000)
			triss = tris + TOUTFSTRING(stats.triangleCount/1000.0f) + U(" k");
		guiTris->setCaption(triss);

		// TODO: TOFIX
		/*
		OverlayElement* guiDbg = OverlayManager::getSingleton().getOverlayElement("Core/DebugText");
		UTFString debugText = "";
		for (int t=0;t<free_truck;t++)
		{
			if (!trucks[t]) continue;
			if (!trucks[t]->debugText.empty())
				debugText += TOSTRING(t) + ": " + trucks[t]->debugText + "\n";
		}
		guiDbg->setCaption(debugText);
		*/

		// create some memory texts
		UTFString memoryText;
		if (TextureManager::getSingleton().getMemoryUsage() > 1)
			memoryText = memoryText + _L("Textures: ") + formatBytes(TextureManager::getSingleton().getMemoryUsage()) + U(" / ") + formatBytes(TextureManager::getSingleton().getMemoryBudget()) + U("\n");
		if (CompositorManager::getSingleton().getMemoryUsage() > 1)
			memoryText = memoryText + _L("Compositors: ") + formatBytes(CompositorManager::getSingleton().getMemoryUsage()) + U(" / ") + formatBytes(CompositorManager::getSingleton().getMemoryBudget()) + U("\n");
		if (FontManager::getSingleton().getMemoryUsage() > 1)
			memoryText = memoryText + _L("Fonts: ") + formatBytes(FontManager::getSingleton().getMemoryUsage()) + U(" / ") + formatBytes(FontManager::getSingleton().getMemoryBudget()) + U("\n");
		if (GpuProgramManager::getSingleton().getMemoryUsage() > 1)
			memoryText = memoryText + _L("GPU Program: ") + formatBytes(GpuProgramManager::getSingleton().getMemoryUsage()) + U(" / ") + formatBytes(GpuProgramManager::getSingleton().getMemoryBudget()) + U("\n");
		if (HighLevelGpuProgramManager ::getSingleton().getMemoryUsage() >1)
			memoryText = memoryText + _L("HL GPU Program: ") + formatBytes(HighLevelGpuProgramManager ::getSingleton().getMemoryUsage()) + U(" / ") + formatBytes(HighLevelGpuProgramManager ::getSingleton().getMemoryBudget()) + U("\n");
		if (MaterialManager::getSingleton().getMemoryUsage() > 1)
			memoryText = memoryText + _L("Materials: ") + formatBytes(MaterialManager::getSingleton().getMemoryUsage()) + U(" / ") + formatBytes(MaterialManager::getSingleton().getMemoryBudget()) + U("\n");
		if (MeshManager::getSingleton().getMemoryUsage() > 1)
			memoryText = memoryText + _L("Meshes: ") + formatBytes(MeshManager::getSingleton().getMemoryUsage()) + U(" / ") + formatBytes(MeshManager::getSingleton().getMemoryBudget()) + U("\n");
		if (SkeletonManager::getSingleton().getMemoryUsage() > 1)
			memoryText = memoryText + _L("Skeletons: ") + formatBytes(SkeletonManager::getSingleton().getMemoryUsage()) + U(" / ") + formatBytes(SkeletonManager::getSingleton().getMemoryBudget()) + U("\n");
		if (MaterialManager::getSingleton().getMemoryUsage() > 1)
			memoryText = memoryText + _L("Materials: ") + formatBytes(MaterialManager::getSingleton().getMemoryUsage()) + U(" / ") + formatBytes(MaterialManager::getSingleton().getMemoryBudget()) + U("\n");
		memoryText = memoryText + U("\n");

		OverlayElement* memoryDbg = OverlayManager::getSingleton().getOverlayElement("Core/MemoryText");
		memoryDbg->setCaption(memoryText);



		float sumMem = TextureManager::getSingleton().getMemoryUsage() + CompositorManager::getSingleton().getMemoryUsage() + FontManager::getSingleton().getMemoryUsage() + GpuProgramManager::getSingleton().getMemoryUsage() + HighLevelGpuProgramManager ::getSingleton().getMemoryUsage() + MaterialManager::getSingleton().getMemoryUsage() + MeshManager::getSingleton().getMemoryUsage() + SkeletonManager::getSingleton().getMemoryUsage() + MaterialManager::getSingleton().getMemoryUsage();
		String sumMemoryText = _L("Memory (Ogre): ") + formatBytes(sumMem) + U("\n");

		OverlayElement* memorySumDbg = OverlayManager::getSingleton().getOverlayElement("Core/CurrMemory");
		memorySumDbg->setCaption(sumMemoryText);

	}
	catch(...)
	{
		// ignore
	}
}

int OverlayWrapper::getDashBoardHeight()
{
	if (!dashboardOverlay) return 0;
	float top = 1 + OverlayManager::getSingleton().getOverlayElement("tracks/dashbar")->getTop() * dashboardOverlay->getScaleY(); // tracks/dashbar top = -0.15 by default
	return (int)(top * (float)win->getHeight());
}

bool OverlayWrapper::mouseMoved(const OIS::MouseEvent& _arg)
{
	if (!airneedlesOverlay->isVisible()) return false;
	bool res = false;
	const OIS::MouseState ms = _arg.state;
	//Beam **trucks = BeamFactory::getSingleton().getTrucks();
	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();

	if (!curr_truck) return res;

	float mouseX = ms.X.abs / (float)ms.width;
	float mouseY = ms.Y.abs / (float)ms.height;

	// TODO: fix: when the window is scaled, the findElementAt doesn not seem to pick up the correct element :-\

	if (curr_truck->driveable == AIRPLANE && ms.buttonDown(OIS::MB_Left))
	{
		OverlayElement *element=airneedlesOverlay->findElementAt(mouseX, mouseY);
		if (element)
		{
			res = true;
			char name[256];
			strcpy(name,element->getName().c_str());
			if (!strncmp(name, "tracks/thrust1", 14)) curr_truck->aeroengines[0]->setThrottle(1.0f-((mouseY-thrtop-throffset)/thrheight));
			if (!strncmp(name, "tracks/thrust2", 14) && curr_truck->free_aeroengine>1) curr_truck->aeroengines[1]->setThrottle(1.0f-((mouseY-thrtop-throffset)/thrheight));
			if (!strncmp(name, "tracks/thrust3", 14) && curr_truck->free_aeroengine>2) curr_truck->aeroengines[2]->setThrottle(1.0f-((mouseY-thrtop-throffset)/thrheight));
			if (!strncmp(name, "tracks/thrust4", 14) && curr_truck->free_aeroengine>3) curr_truck->aeroengines[3]->setThrottle(1.0f-((mouseY-thrtop-throffset)/thrheight));
		}
		//also for main dashboard
		OverlayElement *element2=airdashboardOverlay->findElementAt(mouseX,mouseY);
		if (element2)
		{
			res = true;
			char name[256];
			strcpy(name,element2->getName().c_str());
			//LogManager::getSingleton().logMessage("element "+element2->getName());
			if (!strncmp(name, "tracks/engstart1", 16)) curr_truck->aeroengines[0]->flipStart();
			if (!strncmp(name, "tracks/engstart2", 16) && curr_truck->free_aeroengine>1) curr_truck->aeroengines[1]->flipStart();
			if (!strncmp(name, "tracks/engstart3", 16) && curr_truck->free_aeroengine>2) curr_truck->aeroengines[2]->flipStart();
			if (!strncmp(name, "tracks/engstart4", 16) && curr_truck->free_aeroengine>3) curr_truck->aeroengines[3]->flipStart();
			//heading group
			if (!strcmp(name, "tracks/ap_hdg_but") && curr_truck->autopilot && mTimeUntilNextToggle <= 0)
			{
				mTimeUntilNextToggle = 0.2;
				if (curr_truck->autopilot->toggleHeading(Autopilot::HEADING_FIXED)==Autopilot::HEADING_FIXED)
					OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_but")->setMaterialName("tracks/hdg-on");
				else
					OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_but")->setMaterialName("tracks/hdg-off");
				OverlayManager::getSingleton().getOverlayElement("tracks/ap_wlv_but")->setMaterialName("tracks/wlv-off");
				OverlayManager::getSingleton().getOverlayElement("tracks/ap_nav_but")->setMaterialName("tracks/nav-off");
			}
			if (!strcmp(name, "tracks/ap_wlv_but") && curr_truck->autopilot && mTimeUntilNextToggle <= 0)
			{
				mTimeUntilNextToggle = 0.2;
				if (curr_truck->autopilot->toggleHeading(Autopilot::HEADING_WLV)==Autopilot::HEADING_WLV)
					OverlayManager::getSingleton().getOverlayElement("tracks/ap_wlv_but")->setMaterialName("tracks/wlv-on");
				else
					OverlayManager::getSingleton().getOverlayElement("tracks/ap_wlv_but")->setMaterialName("tracks/wlv-off");
				OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_but")->setMaterialName("tracks/hdg-off");
				OverlayManager::getSingleton().getOverlayElement("tracks/ap_nav_but")->setMaterialName("tracks/nav-off");
			}
			if (!strcmp(name, "tracks/ap_nav_but") && curr_truck->autopilot && mTimeUntilNextToggle <= 0)
			{
				mTimeUntilNextToggle = 0.2;
				if (curr_truck->autopilot->toggleHeading(Autopilot::HEADING_NAV)==Autopilot::HEADING_NAV)
					OverlayManager::getSingleton().getOverlayElement("tracks/ap_nav_but")->setMaterialName("tracks/nav-on");
				else
					OverlayManager::getSingleton().getOverlayElement("tracks/ap_nav_but")->setMaterialName("tracks/nav-off");
				OverlayManager::getSingleton().getOverlayElement("tracks/ap_wlv_but")->setMaterialName("tracks/wlv-off");
				OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_but")->setMaterialName("tracks/hdg-off");
			}
			//altitude group
			if (!strcmp(name, "tracks/ap_alt_but") && curr_truck->autopilot && mTimeUntilNextToggle <= 0)
			{
				mTimeUntilNextToggle = 0.2;
				if (curr_truck->autopilot->toggleAlt(Autopilot::ALT_FIXED)==Autopilot::ALT_FIXED)
					OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_but")->setMaterialName("tracks/hold-on");
				else
					OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_but")->setMaterialName("tracks/hold-off");
				OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_but")->setMaterialName("tracks/vs-off");
			}
			if (!strcmp(name, "tracks/ap_vs_but") && curr_truck->autopilot && mTimeUntilNextToggle <= 0)
			{
				mTimeUntilNextToggle = 0.2;
				if (curr_truck->autopilot->toggleAlt(Autopilot::ALT_VS)==Autopilot::ALT_VS)
					OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_but")->setMaterialName("tracks/vs-on");
				else
					OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_but")->setMaterialName("tracks/vs-off");
				OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_but")->setMaterialName("tracks/hold-off");
			}
			//IAS
			if (!strcmp(name, "tracks/ap_ias_but") && curr_truck->autopilot && mTimeUntilNextToggle <= 0)
			{
				mTimeUntilNextToggle = 0.2;
				if (curr_truck->autopilot->toggleIAS())
					OverlayManager::getSingleton().getOverlayElement("tracks/ap_ias_but")->setMaterialName("tracks/athr-on");
				else
					OverlayManager::getSingleton().getOverlayElement("tracks/ap_ias_but")->setMaterialName("tracks/athr-off");
			}
			//GPWS
			if (!strcmp(name, "tracks/ap_gpws_but") && curr_truck->autopilot && mTimeUntilNextToggle <= 0)
			{
				mTimeUntilNextToggle = 0.2;
				if (curr_truck->autopilot->toggleGPWS())
					OverlayManager::getSingleton().getOverlayElement("tracks/ap_gpws_but")->setMaterialName("tracks/gpws-on");
				else
					OverlayManager::getSingleton().getOverlayElement("tracks/ap_gpws_but")->setMaterialName("tracks/gpws-off");
			}
			//BRKS
			if (!strcmp(name, "tracks/ap_brks_but") && curr_truck->autopilot && mTimeUntilNextToggle <= 0)
			{
				curr_truck->parkingbrakeToggle();
				if (curr_truck->parkingbrake)
					OverlayManager::getSingleton().getOverlayElement("tracks/ap_brks_but")->setMaterialName("tracks/brks-on");
				else
					OverlayManager::getSingleton().getOverlayElement("tracks/ap_brks_but")->setMaterialName("tracks/brks-off");
				mTimeUntilNextToggle = 0.2;
			}
			//trims
			if (!strcmp(name, "tracks/ap_hdg_up") && curr_truck->autopilot && mTimeUntilNextToggle <= 0)
			{
				mTimeUntilNextToggle = 0.1;
				int val=curr_truck->autopilot->adjHDG(1);
				char str[10];
				sprintf(str, "%.3u", val);
				OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_val")->setCaption(str);
			}
			if (!strcmp(name, "tracks/ap_hdg_dn") && curr_truck->autopilot && mTimeUntilNextToggle <= 0)
			{
				mTimeUntilNextToggle = 0.1;
				int val=curr_truck->autopilot->adjHDG(-1);
				char str[10];
				sprintf(str, "%.3u", val);
				OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_val")->setCaption(str);
			}
			if (!strcmp(name, "tracks/ap_alt_up") && curr_truck->autopilot && mTimeUntilNextToggle <= 0)
			{
				mTimeUntilNextToggle = 0.1;
				int val=curr_truck->autopilot->adjALT(100);
				char str[10];
				sprintf(str, "%i00", val/100);
				OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_val")->setCaption(str);
			}
			if (!strcmp(name, "tracks/ap_alt_dn") && curr_truck->autopilot && mTimeUntilNextToggle <= 0)
			{
				mTimeUntilNextToggle = 0.1;
				int val=curr_truck->autopilot->adjALT(-100);
				char str[10];
				sprintf(str, "%i00", val/100);
				OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_val")->setCaption(str);
			}
			if (!strcmp(name, "tracks/ap_vs_up") && curr_truck->autopilot && mTimeUntilNextToggle <= 0)
			{
				mTimeUntilNextToggle = 0.1;
				int val=curr_truck->autopilot->adjVS(100);
				char str[10];
				if (val<0)
					sprintf(str, "%i00", val/100);
				else if (val==0) strcpy(str, "000");
				else sprintf(str, "+%i00", val/100);
				OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_val")->setCaption(str);
			}
			if (!strcmp(name, "tracks/ap_vs_dn") && curr_truck->autopilot && mTimeUntilNextToggle <= 0)
			{
				mTimeUntilNextToggle = 0.1;
				int val=curr_truck->autopilot->adjVS(-100);
				char str[10];
				if (val<0)
					sprintf(str, "%i00", val/100);
				else if (val==0) strcpy(str, "000");
				else sprintf(str, "+%i00", val/100);
				OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_val")->setCaption(str);
			}
			if (!strcmp(name, "tracks/ap_ias_up") && curr_truck->autopilot && mTimeUntilNextToggle <= 0)
			{
				mTimeUntilNextToggle = 0.1;
				int val=curr_truck->autopilot->adjIAS(1);
				char str[10];
				sprintf(str, "%.3u", val);
				OverlayManager::getSingleton().getOverlayElement("tracks/ap_ias_val")->setCaption(str);
			}
			if (!strcmp(name, "tracks/ap_ias_dn") && curr_truck->autopilot && mTimeUntilNextToggle <= 0)
			{
				mTimeUntilNextToggle = 0.1;
				int val=curr_truck->autopilot->adjIAS(-1);
				char str[10];
				sprintf(str, "%.3u", val);
				OverlayManager::getSingleton().getOverlayElement("tracks/ap_ias_val")->setCaption(str);
			}
		}
	}
	return res;
}

bool OverlayWrapper::mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	return mouseMoved(_arg);
}

bool OverlayWrapper::mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	return mouseMoved(_arg);
}
