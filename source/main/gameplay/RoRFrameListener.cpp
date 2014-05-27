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
#include "RoRFrameListener.h"

#include <OgreFontManager.h>

#include <OgreRTShaderSystem.h>
#include <OgreTerrain.h>
#include <OgreTerrainMaterialGeneratorA.h>
#include <OgreTerrainPaging.h>
#include <OgreTerrainQuadTreeNode.h>

#include "AdvancedScreen.h"
#include "Application.h"
#include "AutoPilot.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "BeamFactory.h"
#include "CacheSystem.h"
#include "CameraManager.h"
#include "Character.h"
#include "CharacterFactory.h"
#include "ChatSystem.h"
#include "Collisions.h"
#include "CollisionTools.h"
#include "Dashboard.h"
#include "DashBoardManager.h"
#include "DepthOfFieldEffect.h"
#include "DustManager.h"
#include "EnvironmentMap.h"
#include "ErrorUtils.h"
#include "FlexAirfoil.h"
#include "ForceFeedback.h"
#include "GlowMaterialListener.h"
#include "HDRListener.h"
#include "Heathaze.h"
#include "IHeightFinder.h"
#include "InputEngine.h"
#include "IWater.h"
#include "Language.h"
#include "MainThread.h"
#include "MeshObject.h"
#include "MumbleIntegration.h"
#include "Network.h"
#include "OutProtocol.h"
#include "OverlayWrapper.h"
#include "PlatformUtils.h"
#include "PlayerColours.h"
#include "PreviewRenderer.h"
#include "Replay.h"
#include "Road.h"
#include "Road2.h"
#include "RoRVersion.h"
#include "SceneMouse.h"
#include "ScopeLog.h"
#include "ScrewProp.h"
#include "Scripting.h"
#include "Settings.h"
#include "ShadowManager.h"
#include "SkyManager.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "TruckHUD.h"
#include "TurboProp.h"
#include "Utils.h"
#include "VideoCamera.h"
#include "Water.h"
#include "WriteTextToTexture.h"

#ifdef USE_MYGUI
#include "GUIManager.h"
#include "GUIMenu.h"
#include "GUIFriction.h"
#include "GUIMp.h"
#include "SelectorWindow.h"
#include "LoadingWindow.h"
#include "Console.h"
#include "SurveyMapManager.h"
#include "SurveyMapEntity.h"
#endif //USE_MYGUI

#ifdef USE_MPLATFORM
#include "MPlatformFD.h"
#endif //USE_MPLATFORM

#ifdef FEAT_TIMING
#include "BeamStats.h"
#endif //FEAT_TIMING

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <Windows.h>
#else
#include <stdio.h>
#include <wchar.h>
#endif

// some gcc fixes
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif //OGRE_PLATFORM_LINUX

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
//#include <CFUserNotification.h>
#endif

using namespace Ogre;
using namespace RoR;

bool shutdownall=false;

void RoRFrameListener::startTimer()
{
	raceStartTime = (int)rtime;
	if (ow)
	{
		ow->racing->show();
		ow->laptimes->show();
		ow->laptimems->show();
		ow->laptimemin->show();
	}
}

float RoRFrameListener::stopTimer()
{
	float time=rtime - raceStartTime;
	// let the display on
	if (ow)
	{
		wchar_t txt[256] = L"";
		UTFString fmt = _L("Last lap: %.2i'%.2i.%.2i");
		swprintf(txt, 256, fmt.asWStr_c_str(), ((int)(time))/60,((int)(time))%60, ((int)(time*100.0))%100);
		ow->lasttime->setCaption(UTFString(txt));
		//ow->racing->hide();
		ow->laptimes->hide();
		ow->laptimems->hide();
		ow->laptimemin->hide();
	}
	raceStartTime = -1;
	return time;
}

void RoRFrameListener::updateRacingGUI()
{
	if (!ow) return;
	// update racing gui if required
	float time=rtime - raceStartTime;
	wchar_t txt[10];
	swprintf(txt, 10, L"%.2i", ((int)(time*100.0))%100);
	ow->laptimems->setCaption(txt);
	swprintf(txt, 10, L"%.2i", ((int)(time))%60);
	ow->laptimes->setCaption(txt);
	swprintf(txt, 10, L"%.2i'", ((int)(time))/60);
	ow->laptimemin->setCaption(UTFString(txt));
}

void RoRFrameListener::updateIO(float dt)
{
	Beam *current_truck = BeamFactory::getSingleton().getCurrentTruck();
	if (current_truck && current_truck->driveable == TRUCK)
	{


		// force feedback
		if (forcefeedback)
		{
			int cameranodepos = 0;
			int cameranodedir = 0;
			int cameranoderoll = 0;

			if (current_truck->cameranodepos[0] >= 0 && current_truck->cameranodepos[0] < MAX_NODES)
				cameranodepos = current_truck->cameranodepos[0];
			if (current_truck->cameranodedir[0] >= 0 && current_truck->cameranodedir[0] < MAX_NODES)
				cameranodedir = current_truck->cameranodedir[0];
			if (current_truck->cameranoderoll[0] >= 0 && current_truck->cameranoderoll[0] < MAX_NODES)
				cameranoderoll = current_truck->cameranoderoll[0];

			Vector3 udir = current_truck->nodes[cameranodepos].RelPosition-current_truck->nodes[cameranodedir].RelPosition;
			Vector3 uroll = current_truck->nodes[cameranodepos].RelPosition-current_truck->nodes[cameranoderoll].RelPosition;
			
			udir.normalise();
			uroll.normalise();

			forcefeedback->setForces(-current_truck->ffforce.dotProduct(uroll)/10000.0,
				current_truck->ffforce.dotProduct(udir)/10000.0,
				current_truck->WheelSpeed,
				current_truck->hydrodircommand,
				current_truck->ffhydro);
		}
	}
}

// Constructor takes a RenderWindow because it uses that to determine input context
RoRFrameListener::RoRFrameListener(
	RoR::MainThread * main_thread_control
) :
	clutch(0),
	dashboard(0),
	dof(0),
	flaresMode(3), // on by default
	forcefeedback(0),
	freeTruckPosition(false),
	heathaze(0),
	hidegui(false),
	initialized(false),
	loading_state(NONE_LOADED),
	mStatsOn(0),
	mTimeUntilNextToggle(0),
	mTruckInfoOn(false),
	netChat(0),
	netPointToUID(-1),
	netcheckGUITimer(0),
	ow(0),
	m_main_thread_control(main_thread_control),
	persostart(Vector3(0,0,0)),
	pressure_pressed(false),
	raceStartTime(-1),
	reload_box(0),
	rtime(0)
{

	ow = RoR::Application::GetOverlayWrapper();

	enablePosStor = BSETTING("Position Storage", false);

#ifdef USE_MPLATFORM
	mplatform = new MPlatform_FD();
	if (mplatform) mplatform->connect();
#endif

	//network
	bool enableNetwork = BSETTING("Network enable", false);

	// setup direction arrow overlay
	dirvisible = false;
	dirArrowPointed = Vector3::ZERO;

	windowResized(gEnv->renderWindow);
	RoRWindowEventUtilities::addWindowEventListener(gEnv->renderWindow, this);

	// get lights mode
	String lightsMode = SSETTING("Lights", "Only current vehicle, main lights");
	if (lightsMode == "None (fastest)")
		flaresMode = 0;
	else if (lightsMode == "No light sources")
		flaresMode = 1;
	else if (lightsMode == "Only current vehicle, main lights")
		flaresMode = 2;
	else if (lightsMode == "All vehicles, main lights")
		flaresMode = 3;
	else if (lightsMode == "All vehicles, all lights")
		flaresMode = 4;

	// heathaze effect
	if (BSETTING("HeatHaze", false))
	{
		heathaze = new HeatHaze();
		heathaze->setEnable(true);
	}



	// force feedback
	if (BSETTING("Force Feedback", true))
	{
		//check if a device has been detected
		if (RoR::Application::GetInputEngine()->getForceFeedbackDevice())
		{
			//retrieve gain values
			float ogain   = FSETTING("Force Feedback Gain",      100) / 100.0f;
			float stressg = FSETTING("Force Feedback Stress",    100) / 100.0f;
			float centg   = FSETTING("Force Feedback Centering", 0  ) / 100.0f;
			float camg    = FSETTING("Force Feedback Camera",    100) / 100.0f;

			forcefeedback = new ForceFeedback(RoR::Application::GetInputEngine()->getForceFeedbackDevice(), ogain, stressg, centg, camg);
		}
	}

	String screenshotFormatString = SSETTING("Screenshot Format", "jpg (smaller, default)");
	if     (screenshotFormatString == "jpg (smaller, default)")
		strcpy(screenshotformat, "jpg");
	else if (screenshotFormatString =="png (bigger, no quality loss)")
		strcpy(screenshotformat, "png");
	else
		strncpy(screenshotformat, screenshotFormatString.c_str(), 10);


}

RoRFrameListener::~RoRFrameListener()
{
#ifdef USE_MYGUI
	LoadingWindow::freeSingleton();
	SelectorWindow::freeSingleton();
#endif //MYGUI

//	if (joy) delete (joy);

#ifdef USE_SOCKETW
	if (gEnv->network) delete (gEnv->network);
#endif //SOCKETW
	//we should destroy OIS here
	//we could also try to destroy SoundScriptManager, but we don't care!
	#ifdef USE_MPLATFORM
	if (mplatform)
	{
		if (mplatform->disconnect()) delete(mplatform);
	}
	#endif
}



void RoRFrameListener::updateCruiseControl(Beam* curr_truck, float dt)
{
	if (RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_BRAKE) > 0.05f ||
		RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_MANUAL_CLUTCH) > 0.05f ||
		(curr_truck->cc_target_speed < curr_truck->cc_target_speed_lower_limit) ||
		(curr_truck->parkingbrake && curr_truck->engine->getGear() > 0) ||
		!curr_truck->engine->isRunning() ||
		!curr_truck->engine->hasContact())
	{
		curr_truck->cruisecontrolToggle();
		return;
	}

	if (curr_truck->engine->getGear() > 0)
	{
		// Try to maintain the target speed
		if (curr_truck->cc_target_speed > curr_truck->WheelSpeed)
		{
			float accl = (curr_truck->cc_target_speed - curr_truck->WheelSpeed) * 2.0f;
			accl = std::max(curr_truck->engine->getAcc(), accl);
			accl = std::min(accl, 1.0f);
			curr_truck->engine->setAcc(accl);
		}
	} else if (curr_truck->engine->getGear() == 0) // out of gear
	{
		// Try to maintain the target rpm
		if (curr_truck->cc_target_rpm > curr_truck->engine->getRPM())
		{
			float accl = (curr_truck->cc_target_rpm - curr_truck->engine->getRPM()) * 0.01f;
			accl = std::max(curr_truck->engine->getAcc(), accl);
			accl = std::min(accl, 1.0f);
			curr_truck->engine->setAcc(accl);
		}
	}

	if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_ACCL))
	{
		if (curr_truck->engine->getGear() > 0)
		{
			curr_truck->cc_target_speed += 2.5f * dt;
			curr_truck->cc_target_speed  = std::max(curr_truck->cc_target_speed_lower_limit, curr_truck->cc_target_speed);
			if (curr_truck->sl_enabled)
			{
				curr_truck->cc_target_speed  = std::min(curr_truck->cc_target_speed, curr_truck->sl_speed_limit);
			}
		} else if (curr_truck->engine->getGear() == 0) // out of gear
		{
			curr_truck->cc_target_rpm += 1000.0f * dt;
			curr_truck->cc_target_rpm  = std::min(curr_truck->cc_target_rpm, curr_truck->engine->getMaxRPM());
		}
	}
	if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_DECL))
	{
		if (curr_truck->engine->getGear() > 0)
		{
			curr_truck->cc_target_speed -= 2.5f * dt;
			curr_truck->cc_target_speed  = std::max(curr_truck->cc_target_speed_lower_limit, curr_truck->cc_target_speed);
		} else if (curr_truck->engine->getGear() == 0) // out of gear
		{
			curr_truck->cc_target_rpm -= 1000.0f * dt;
			curr_truck->cc_target_rpm  = std::max(curr_truck->engine->getMinRPM(), curr_truck->cc_target_rpm);
		}
	}
	if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_READJUST))
	{
		curr_truck->cc_target_speed = std::max(curr_truck->WheelSpeed, curr_truck->cc_target_speed);
		if (curr_truck->sl_enabled)
		{
			curr_truck->cc_target_speed = std::min(curr_truck->cc_target_speed, curr_truck->sl_speed_limit);
		}
		curr_truck->cc_target_rpm   = curr_truck->engine->getRPM();
	}

	if (curr_truck->cc_can_brake)
	{
		if (curr_truck->WheelSpeed > curr_truck->cc_target_speed + 0.5f && !RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_ACCELERATE))
		{
			float brake = (curr_truck->WheelSpeed - curr_truck->cc_target_speed) * 0.5f;
			brake = std::min(brake, 1.0f);
			curr_truck->brake = curr_truck->brakeforce * brake;
		}
	}
}

void RoRFrameListener::checkSpeedlimit(Beam* curr_truck, float dt)
{
	if (curr_truck->sl_enabled && curr_truck->engine->getGear() != 0)
	{
		float accl = (curr_truck->sl_speed_limit - std::abs(curr_truck->WheelSpeed)) * 2.0f;
		accl = std::max(0.0f, accl);
		accl = std::min(accl, curr_truck->engine->getAcc());
		curr_truck->engine->setAcc(accl);
	}
}

bool RoRFrameListener::updateEvents(float dt)
{
	if (dt==0.0f) return true;

	RoR::Application::GetInputEngine()->updateKeyBounces(dt);
	if (!RoR::Application::GetInputEngine()->getInputsChanged()) return true;

	bool dirty = false;
	//update joystick readings
	//	joy->UpdateInputState();

	//stick shift general uglyness
	/*
	// no more stickshift, commented out when upgrading to the inputengine
	if (loading_state==ALL_LOADED && current_truck!=-1 && trucks[current_truck]->driveable==TRUCK && trucks[current_truck]->engine->getAutoMode()==MANUAL)
	{
		int gb;
		gb=joy->updateStickShift(true, trucks[current_truck]->engine->getClutch());
		// TODO: FIXME
		//if (gb!=-1) trucks[current_truck]->engine->setGear(gb);
	}
	else joy->updateStickShift(false, 0.0);
	*/

	// update overlays if enabled
	if (ow) ow->update(dt);

	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();

#ifdef USE_MYGUI
	if (GUI_Friction::getSingletonPtr() && GUI_Friction::getSingleton().getVisible() && curr_truck)
	{
		// friction GUI active
		ground_model_t *gm = curr_truck->getLastFuzzyGroundModel();
		if (gm)
			GUI_Friction::getSingleton().setActiveCol(gm);
	}
#endif //USE_MYGUI

#ifdef USE_MYGUI
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_CHATMODE, 0.5f) && !hidegui)
	{
		Console *c = RoR::Application::GetConsole();
		if (c)
		{
			RoR::Application::GetInputEngine()->resetKeys();
			c->setVisible(true);
			c->select();
		}
	}
#endif //USE_MYGUI
	// update characters
	if (loading_state==ALL_LOADED && gEnv->network)
	{
		CharacterFactory::getSingleton().updateCharacters(dt);
	} else if (loading_state==ALL_LOADED && !gEnv->network)
	{
		gEnv->player->update(dt);
	}

	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUIT_GAME))
	{
		shutdown_final();
	}

	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_SCREENSHOT, 0.5f))
	{
		int mNumScreenShots=0;
		String tmpfn = SSETTING("User Path", "") + String("screenshot_") + TOSTRING(++mNumScreenShots) + String(".") + String(screenshotformat);
		while(RoR::PlatformUtils::FileExists(tmpfn.c_str()))
		{
			tmpfn = SSETTING("User Path", "") + String("screenshot_") + TOSTRING(++mNumScreenShots) + String(".") + String(screenshotformat);
		}

#ifdef USE_MYGUI
		MyGUI::PointerManager::getInstance().setVisible(false);
#endif // USE_MYGUI
		if (String(screenshotformat) == "png")
		{


			// add some more data into the image
			AdvancedScreen *as = new AdvancedScreen(gEnv->renderWindow, tmpfn);
			//as->addData("terrain_Name", loadedTerrain);
			//as->addData("terrain_ModHash", terrainModHash);
			//as->addData("terrain_FileHash", terrainFileHash);
			as->addData("Truck_Num", TOSTRING(BeamFactory::getSingleton().getCurrentTruckNumber()));
			if (curr_truck)
			{
				as->addData("Truck_fname", curr_truck->realtruckfilename);
				as->addData("Truck_name", curr_truck->getTruckName());
				as->addData("Truck_beams", TOSTRING(curr_truck->getBeamCount()));
				as->addData("Truck_nodes", TOSTRING(curr_truck->getNodeCount()));
			}
			as->addData("User_NickName", SSETTING("Nickname", "Anonymous"));
			as->addData("User_Language", SSETTING("Language", "English"));
			as->addData("RoR_VersionString", String(ROR_VERSION_STRING));
			as->addData("RoR_VersionSVN", String(SVN_REVISION));
			as->addData("RoR_VersionSVNID", String(SVN_ID));
			as->addData("RoR_ProtocolVersion", String(RORNET_VERSION));
			as->addData("RoR_BinaryHash", SSETTING("BinaryHash", ""));
			as->addData("MP_ServerName", SSETTING("Server name", ""));
			as->addData("MP_ServerPort", SSETTING("Server port", ""));
			as->addData("MP_NetworkEnabled", SSETTING("Network enable", "No"));
			as->addData("Camera_Mode", gEnv->cameraManager ? TOSTRING(gEnv->cameraManager->getCurrentBehavior()) : "None");
			as->addData("Camera_Position", TOSTRING(gEnv->mainCamera->getPosition()));

			const RenderTarget::FrameStats& stats = gEnv->renderWindow->getStatistics();
			as->addData("AVGFPS", TOSTRING(stats.avgFPS));

			as->write();
			delete(as);
		} else
		{
			gEnv->renderWindow->update();
			gEnv->renderWindow->writeContentsToFile(tmpfn);
		}

#ifdef USE_MYGUI
		MyGUI::PointerManager::getInstance().setVisible(true);
#endif // USE_MYGUI

		// show new flash message
		String ssmsg = _L("wrote screenshot:") + TOSTRING(mNumScreenShots);
		LOG(ssmsg);
#ifdef USE_MYGUI
		RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "camera.png", 10000, false);
#endif //USE_MYGUI
	}

	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCKEDIT_RELOAD, 0.5f) && curr_truck)
	{
		reloadCurrentTruck();
		return true;
	}

	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_GETNEWVEHICLE, 0.5f) && loading_state != NONE_LOADED)
	{
		// get out first
		if (curr_truck) BeamFactory::getSingleton().setCurrentTruck(-1);
		reload_pos = gEnv->player->getPosition() + Vector3(0.0f, 1.0f, 0.0f); // 1 meter above the character
		freeTruckPosition = true;
		loading_state = RELOADING;
		SelectorWindow::getSingleton().show(SelectorWindow::LT_AllBeam);
		return true;
	}

	// position storage
	if (enablePosStor && curr_truck)
	{
		int res = -10, slot=-1;
		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS01, 0.5f)) { slot=0; res = curr_truck->savePosition(slot); };
		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS02, 0.5f)) { slot=1; res = curr_truck->savePosition(slot); };
		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS03, 0.5f)) { slot=2; res = curr_truck->savePosition(slot); };
		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS04, 0.5f)) { slot=3; res = curr_truck->savePosition(slot); };
		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS05, 0.5f)) { slot=4; res = curr_truck->savePosition(slot); };
		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS06, 0.5f)) { slot=5; res = curr_truck->savePosition(slot); };
		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS07, 0.5f)) { slot=6; res = curr_truck->savePosition(slot); };
		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS08, 0.5f)) { slot=7; res = curr_truck->savePosition(slot); };
		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS09, 0.5f)) { slot=8; res = curr_truck->savePosition(slot); };
		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS10, 0.5f)) { slot=9; res = curr_truck->savePosition(slot); };
#ifdef USE_MYGUI
		if (slot != -1 && !res)
			RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Position saved under slot ") + TOSTRING(slot+1), "infromation.png");
		else if (slot != -1 && res)
			RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Error while saving position saved under slot ") + TOSTRING(slot+1), "error.png");
#endif //USE_MYGUI

		if (res == -10)
		{
			if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS01, 0.5f)) { slot=0; res = curr_truck->loadPosition(slot); };
			if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS02, 0.5f)) { slot=1; res = curr_truck->loadPosition(slot); };
			if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS03, 0.5f)) { slot=2; res = curr_truck->loadPosition(slot); };
			if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS04, 0.5f)) { slot=3; res = curr_truck->loadPosition(slot); };
			if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS05, 0.5f)) { slot=4; res = curr_truck->loadPosition(slot); };
			if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS06, 0.5f)) { slot=5; res = curr_truck->loadPosition(slot); };
			if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS07, 0.5f)) { slot=6; res = curr_truck->loadPosition(slot); };
			if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS08, 0.5f)) { slot=7; res = curr_truck->loadPosition(slot); };
			if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS09, 0.5f)) { slot=8; res = curr_truck->loadPosition(slot); };
			if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS10, 0.5f)) { slot=9; res = curr_truck->loadPosition(slot); };
#ifdef USE_MYGUI
			if (slot != -1 && res==0)
				RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Loaded position from slot ") + TOSTRING(slot+1), "infromation.png");
			else if (slot != -1 && res!=0)
				RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Could not load position from slot ") + TOSTRING(slot+1), "error.png");
#endif // USE_MYGUI
		}
	}

	// camera FOV settings
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_FOV_LESS, 0.1f) || RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_FOV_MORE, 0.1f))
	{
		int fov = gEnv->mainCamera->getFOVy().valueDegrees();

		if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_COMMON_FOV_LESS))
			fov--;
		else
			fov++;

		if (fov >= 10 && fov <= 160)
		{
			gEnv->mainCamera->setFOVy(Degree(fov));

	#ifdef USE_MYGUI
			RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("FOV: ") + TOSTRING(fov), "camera_edit.png", 2000);
	#endif // USE_MYGUI

			// save the settings
			if (gEnv->cameraManager &&
				gEnv->cameraManager->hasActiveBehavior() &&
				gEnv->cameraManager->getCurrentBehavior() == CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM)
			{
				SETTINGS.setSetting("FOV Internal", TOSTRING(fov));
			} else
			{
				SETTINGS.setSetting("FOV External", TOSTRING(fov));
			}
		} else
		{
#ifdef USE_MYGUI
			RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("FOV: Limit reached"), "camera_edit.png", 2000);
#endif // USE_MYGUI
		}
	}

	// full screen/windowed screen switching
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_FULLSCREEN_TOGGLE, 2.0f))
	{
		static int org_width = -1, org_height = -1;
		int width = gEnv->renderWindow->getWidth();
		int height = gEnv->renderWindow->getHeight();
		if (org_width == -1)
			org_width = width;
		if (org_height == -1)
			org_height = height;
		bool mode = gEnv->renderWindow->isFullScreen();
		if (!mode)
		{
			gEnv->renderWindow->setFullscreen(true, org_width, org_height);
			LOG(" ** switched to fullscreen: "+TOSTRING(width)+"x"+TOSTRING(height));
		} else
		{
			gEnv->renderWindow->setFullscreen(false, 640, 480);
			gEnv->renderWindow->setFullscreen(false, org_width, org_height);
			LOG(" ** switched to windowed mode: ");
		}
	}


	if (loading_state==ALL_LOADED || loading_state == TERRAIN_EDITOR)
	{
		if (gEnv->cameraManager && !gEnv->cameraManager->gameControlsLocked())
		{
			if (!curr_truck)
			{
				if (gEnv->player)
				{
					gEnv->player->setPhysicsEnabled(true);
				}
			} else // we are in a vehicle
			{
				// get commands
				// -- here we should define a maximum numbers per trucks. Some trucks does not have that much commands
				// -- available, so why should we iterate till MAX_COMMANDS?
				for (int i=1; i<=MAX_COMMANDS+1; i++)
				{
					int eventID = EV_COMMANDS_01 + (i - 1);

					curr_truck->commandkey[i].playerInputValue = RoR::Application::GetInputEngine()->getEventValue(eventID);
				}

				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_FORWARDCOMMANDS))
				{
					curr_truck->forwardcommands = !curr_truck->forwardcommands;
#ifdef USE_MYGUI
					if ( curr_truck->forwardcommands )
					{
						RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("forwardcommands enabled"), "information.png", 3000);
					} else
					{
						RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("forwardcommands disabled"), "information.png", 3000);
					}
#endif // USE_MYGUI
				}
				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_IMPORTCOMMANDS))
				{
					curr_truck->importcommands = !curr_truck->importcommands;
#ifdef USE_MYGUI
					if ( curr_truck->importcommands )
					{
						RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("importcommands enabled"), "information.png", 3000);
					} else
					{
						RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("importcommands disabled"), "information.png", 3000);
					}
#endif // USE_MYGUI
				}

				// replay mode
				if (curr_truck->replaymode)
				{
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FORWARD, 0.1f) && curr_truck->replaypos <= 0)
					{
						curr_truck->replaypos++;
					}
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_BACKWARD, 0.1f) && curr_truck->replaypos > -curr_truck->replaylen)
					{
						curr_truck->replaypos--;
					}
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_FORWARD, 0.1f) && curr_truck->replaypos+10 <= 0)
					{
						curr_truck->replaypos+=10;
					}
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_BACKWARD, 0.1f) && curr_truck->replaypos-10 > -curr_truck->replaylen)
					{
						curr_truck->replaypos-=10;
					}

					if (RoR::Application::GetInputEngine()->isKeyDown(OIS::KC_LMENU))
					{
						if (curr_truck->replaypos <= 0 && curr_truck->replaypos >= -curr_truck->replaylen)
						{
							if (RoR::Application::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) || RoR::Application::GetInputEngine()->isKeyDown(OIS::KC_RSHIFT))
							{
								curr_truck->replaypos += RoR::Application::GetInputEngine()->getMouseState().X.rel * 1.5f;
							} else
							{
								curr_truck->replaypos += RoR::Application::GetInputEngine()->getMouseState().X.rel * 0.05f;
							}
							if (curr_truck->replaypos > 0)
							{
								curr_truck->replaypos = 0;
							}
							if (curr_truck->replaypos < -curr_truck->replaylen)
							{
								curr_truck->replaypos = -curr_truck->replaylen;
							}
						}
					}
				}

				if (curr_truck->driveable==TRUCK)
				{
					if (!curr_truck->replaymode)
					{
						if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LEFT_MIRROR_LEFT))
							curr_truck->leftMirrorAngle-=0.001;

						if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LEFT_MIRROR_RIGHT))
							curr_truck->leftMirrorAngle+=0.001;

						if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_RIGHT_MIRROR_LEFT))
							curr_truck->rightMirrorAngle-=0.001;

						if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_RIGHT_MIRROR_RIGHT))
							curr_truck->rightMirrorAngle+=0.001;

					} // end of (!curr_truck->replaymode) block

					if (!curr_truck->replaymode)
					{
						// steering
						float tmp_left_digital  = RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_STEER_LEFT,  false, InputEngine::ET_DIGITAL);
						float tmp_right_digital = RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_STEER_RIGHT, false, InputEngine::ET_DIGITAL);
						float tmp_left_analog   = RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_STEER_LEFT,  false, InputEngine::ET_ANALOG);
						float tmp_right_analog  = RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_STEER_RIGHT, false, InputEngine::ET_ANALOG);

						float sum = -std::max(tmp_left_digital,tmp_left_analog)+ std::max(tmp_right_digital,tmp_right_analog);

						if (sum < -1) sum = -1;
						if (sum > 1) sum = 1;

						curr_truck->hydrodircommand = sum;

						if ((tmp_left_digital < tmp_left_analog) || (tmp_right_digital < tmp_right_analog))
						{
							curr_truck->hydroSpeedCoupling = false;
						} else
						{
							curr_truck->hydroSpeedCoupling = true;
						}

						if (curr_truck->engine)
						{
							bool arcadeControls = BSETTING("ArcadeControls", false);

							float accl  = RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_ACCELERATE);
							float brake = RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_BRAKE);

							// arcade controls are only working with auto-clutch!
							if (!arcadeControls || curr_truck->engine->getAutoMode() > BeamEngine::SEMIAUTO)
							{
								// classic mode, realistic
								curr_truck->engine->autoSetAcc(accl);
								curr_truck->brake = brake * curr_truck->brakeforce;
							} else
							{
								// start engine
								if (curr_truck->engine->hasContact() && !curr_truck->engine->isRunning() && (accl > 0 || brake > 0))
								{
									curr_truck->engine->start();
								}

								// arcade controls: hey - people wanted it x| ... <- and it's convenient
								if (curr_truck->engine->getGear() >= 0)
								{
									// neutral or drive forward, everything is as its used to be: brake is brake and accel. is accel.
									curr_truck->engine->autoSetAcc(accl);
									curr_truck->brake = brake * curr_truck->brakeforce;
								} else
								{
									// reverse gear, reverse controls: brake is accel. and accel. is brake.
									curr_truck->engine->autoSetAcc(brake);
									curr_truck->brake = accl * curr_truck->brakeforce;
								}

								// only when the truck really is not moving anymore
								if (fabs(curr_truck->WheelSpeed) <= 1.0f)
								{
									float velocity = 0.0f;

									if (curr_truck->cameranodepos[0] >= 0 && curr_truck->cameranodedir[0] >= 0)
									{
										Vector3 hdir = (curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition - curr_truck->nodes[curr_truck->cameranodedir[0]].RelPosition).normalisedCopy();
										velocity = hdir.dotProduct(curr_truck->nodes[0].Velocity);
									}

									// switching point, does the user want to drive forward from backward or the other way round? change gears?
									if (velocity < 1.0f && brake > 0.5f && accl < 0.5f && curr_truck->engine->getGear() > 0)
									{
										// we are on the brake, jump to reverse gear
										if (curr_truck->engine->getAutoMode() == BeamEngine::AUTOMATIC)
										{
											curr_truck->engine->autoShiftSet(BeamEngine::REAR);
										} else
										{
											curr_truck->engine->setGear(-1);
										}
									} else if (velocity > -1.0f && brake < 0.5f && accl > 0.5f && curr_truck->engine->getGear() < 0)
									{
										// we are on the gas pedal, jump to first gear when we were in rear gear
										if (curr_truck->engine->getAutoMode() == BeamEngine::AUTOMATIC)
										{									
											curr_truck->engine->autoShiftSet(BeamEngine::DRIVE);
										} else
										{
											curr_truck->engine->setGear(1);
										}
									}
								}
							}

							// IMI
							// gear management -- it might, should be transferred to a standalone function of Beam or RoRFrameListener
							if (curr_truck->engine->getAutoMode() == BeamEngine::AUTOMATIC)
							{
								if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_AUTOSHIFT_UP))
								{
									curr_truck->engine->autoShiftUp();
								}
								if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_AUTOSHIFT_DOWN))
								{
									curr_truck->engine->autoShiftDown();
								}
							}

							if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_CONTACT))
							{
								curr_truck->engine->toggleContact();
							}

							if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_STARTER) && curr_truck->engine->hasContact())
							{
								// starter
								curr_truck->engine->setstarter(1);
#ifdef USE_OPENAL
								SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_STARTER);
#endif // OPENAL
							} else
							{
								curr_truck->engine->setstarter(0);
#ifdef USE_OPENAL
								SoundScriptManager::getSingleton().trigStop(curr_truck, SS_TRIG_STARTER);
#endif // OPENAL
							}

							if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SWITCH_SHIFT_MODES))
							{
								// toggle Auto shift
								curr_truck->engine->toggleAutoMode();

								// force gui update
								curr_truck->triggerGUIFeaturesChanged();
#ifdef USE_MYGUI
								switch(curr_truck->engine->getAutoMode())
								{
									case BeamEngine::AUTOMATIC:
										RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Automatic shift"), "cog.png", 3000);
										break;
									case BeamEngine::SEMIAUTO:
										RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Manual shift - Auto clutch"), "cog.png", 3000);
										break;
									case BeamEngine::MANUAL:
										RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Fully Manual: sequential shift"), "cog.png", 3000);
										break;
									case BeamEngine::MANUAL_STICK:
										RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Fully manual: stick shift"), "cog.png", 3000);
										break;
									case BeamEngine::MANUAL_RANGES:
										RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Fully Manual: stick shift with ranges"), "cog.png", 3000);
										break;
								}
#endif //USE_MYGUI
							}

							// joy clutch
							float cval = RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_MANUAL_CLUTCH);
							curr_truck->engine->setManualClutch(cval);

							bool gear_changed_rel = false;
							int shiftmode = curr_truck->engine->getAutoMode();

							if (shiftmode <= BeamEngine::MANUAL) // auto, semi auto and sequential shifting
							{
								if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_UP))
								{
									if (shiftmode != BeamEngine::AUTOMATIC || curr_truck->engine->getAutoShift() == BeamEngine::DRIVE)
									{
										curr_truck->engine->shift(1);
										gear_changed_rel = true;
									}
								} else if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_DOWN))
								{
									if (shiftmode  > BeamEngine::SEMIAUTO ||
										shiftmode == BeamEngine::SEMIAUTO  && !arcadeControls ||
										shiftmode == BeamEngine::SEMIAUTO  && curr_truck->engine->getGear() > 0 ||
										shiftmode == BeamEngine::AUTOMATIC && curr_truck->engine->getGear() > 1)
									{
										curr_truck->engine->shift(-1);
										gear_changed_rel = true;
									}
								} else if (shiftmode != BeamEngine::AUTOMATIC && RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_NEUTRAL))
								{
									curr_truck->engine->shiftTo(0);
								}
							} else //if (shiftmode > BeamEngine::MANUAL) // h-shift or h-shift with ranges shifting
							{
								bool gear_changed = false;
								bool found        = false;
								int curgear       = curr_truck->engine->getGear();
								int curgearrange    = curr_truck->engine->getGearRange();
								int gearoffset      = std::max(0, curgear - curgearrange * 6);

								// one can select range only if in neutral
								if (shiftmode==BeamEngine::MANUAL_RANGES && curgear == 0)
								{
									//  maybe this should not be here, but should experiment
									if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_LOWRANGE) && curgearrange != 0)
									{
										curr_truck->engine->setGearRange(0);
										gear_changed = true;
#ifdef USE_MYGUI
										RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Low range selected"), "cog.png", 3000);
#endif //USE_MYGUI
									} else if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_MIDRANGE)  && curgearrange != 1 && curr_truck->engine->getNumGearsRanges()>1)
									{
										curr_truck->engine->setGearRange(1);
										gear_changed = true;
#ifdef USE_MYGUI
										RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Mid range selected"), "cog.png", 3000);
#endif //USE_MYGUI
									} else if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_HIGHRANGE) && curgearrange != 2 && curr_truck->engine->getNumGearsRanges()>2)
									{
										curr_truck->engine->setGearRange(2);
										gear_changed = true;
#ifdef USE_MYGUI
										RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("High range selected"), "cog.png", 3000);
#endif // USE_MYGUI
									}
								}
//zaxxon
								if (curgear == -1)
								{
									gear_changed = !RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR_REVERSE);
								} else if (curgear > 0 && curgear < 19)
								{
									if (shiftmode==BeamEngine::MANUAL)
									{
										gear_changed = !RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR01 + curgear -1);
									} else
									{
										gear_changed = !RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR01 + gearoffset-1); // range mode
									}
								}

								if (gear_changed || curgear == 0)
								{
									if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR_REVERSE))
									{
										curr_truck->engine->shiftTo(-1);
										found = true;
									} else if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_NEUTRAL))
									{
										curr_truck->engine->shiftTo(0);
										found = true;
									} else
									{
										if (shiftmode == BeamEngine::MANUAL_STICK)
										{
											for (int i=1; i < 19 && !found; i++)
											{
												if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR01 + i - 1))
												{
													curr_truck->engine->shiftTo(i);
													found = true;
												}
											}
										} else // BeamEngine::MANUALMANUAL_RANGES
										{
											for (int i=1; i < 7 && !found; i++)
											{
												if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR01 + i - 1))
												{
													curr_truck->engine->shiftTo(i + curgearrange * 6);
													found = true;
												}
											}
										}
									}
									if (!found)
									{
										curr_truck->engine->shiftTo(0);
									}
								} // end of if (gear_changed)
							} // end of shitmode > BeamEngine::MANUAL

							// anti roll back in BeamEngine::AUTOMATIC (DRIVE, TWO, ONE) mode
							if (curr_truck->engine->getAutoMode()  == BeamEngine::AUTOMATIC &&
								(curr_truck->engine->getAutoShift() == BeamEngine::DRIVE ||
								curr_truck->engine->getAutoShift() == BeamEngine::TWO ||
								curr_truck->engine->getAutoShift() == BeamEngine::ONE) &&
								curr_truck->WheelSpeed < +0.1f)
							{
								Vector3 dirDiff = (curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition - curr_truck->nodes[curr_truck->cameranodedir[0]].RelPosition).normalisedCopy();
								Degree pitchAngle = Radian(asin(dirDiff.dotProduct(Vector3::UNIT_Y)));

								if (pitchAngle.valueDegrees() > +1.0f)
								{
									if (sin(pitchAngle.valueRadians()) * curr_truck->getTotalMass() > curr_truck->engine->getTorque() / 2.0f)
									{
										curr_truck->brake = curr_truck->brakeforce;
									} else
									{
										curr_truck->brake = curr_truck->brakeforce * (1.0f - accl);
									}
								}
							}

							// anti roll forth in BeamEngine::AUTOMATIC (REAR) mode
							if (curr_truck->engine->getAutoMode()  == BeamEngine::AUTOMATIC &&
								curr_truck->engine->getAutoShift() == BeamEngine::REAR &&
								curr_truck->WheelSpeed > -0.1f)
							{
								Vector3 dirDiff = (curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition - curr_truck->nodes[curr_truck->cameranodedir[0]].RelPosition).normalisedCopy();
								Degree pitchAngle = Radian(asin(dirDiff.dotProduct(Vector3::UNIT_Y)));
								float accl = RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_ACCELERATE);

								if (pitchAngle.valueDegrees() < -1.0f)
								{
									if (sin(pitchAngle.valueRadians()) * curr_truck->getTotalMass() < curr_truck->engine->getTorque() / 2.0f)
									{
										curr_truck->brake = curr_truck->brakeforce;
									} else
									{
										curr_truck->brake = curr_truck->brakeforce * (1.0f - accl);
									}
								}
							}
						} // end of ->engine
#ifdef USE_OPENAL
						if (curr_truck->brake > curr_truck->brakeforce / 6.0f)
						{
							SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_BRAKE);
						} else
						{
							SoundScriptManager::getSingleton().trigStop(curr_truck, SS_TRIG_BRAKE);
						}
#endif // USE_OPENAL
					} // end of ->replaymode

					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_AXLE_LOCK))
					{
						// toggle auto shift
						if (!curr_truck->getAxleLockCount())
						{
#ifdef USE_MYGUI
							RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("No differential installed on current vehicle!"), "warning.png", 3000);
#endif // USE_MYGUI
						} else
						{
							curr_truck->toggleAxleLock();
#ifdef USE_MYGUI
							RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Differentials switched to: ") + curr_truck->getAxleLockName(), "cog.png", 3000);
#endif // USE_MYGUI
						}
					}

#ifdef USE_OPENAL
					if (curr_truck->ispolice)
					{
						if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_HORN))
						{
							SoundScriptManager::getSingleton().trigToggle(curr_truck, SS_TRIG_HORN);
						}
					} else
					{
						if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_HORN) && !curr_truck->replaymode)
						{
							SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_HORN);
						} else
						{
							SoundScriptManager::getSingleton().trigStop(curr_truck, SS_TRIG_HORN);
						}
					}
#endif // OPENAL

					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_PARKING_BRAKE))
					{
						curr_truck->parkingbrakeToggle();
					}

					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_ANTILOCK_BRAKE))
					{
						if (curr_truck->alb_present && !curr_truck->alb_notoggle)
						{
							curr_truck->antilockbrakeToggle();
						}
					}

					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TRACTION_CONTROL))
					{
						if (curr_truck->tc_present && !curr_truck->tc_notoggle) curr_truck->tractioncontrolToggle();
					}

					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_CRUISE_CONTROL))
					{
						curr_truck->cruisecontrolToggle();
					}
					if (curr_truck->cc_mode)
					{

						updateCruiseControl(curr_truck, dt);
					}
					checkSpeedlimit(curr_truck, dt);
				}
				if (curr_truck->driveable==AIRPLANE)
				{
					//autopilot
					if (curr_truck->autopilot && curr_truck->autopilot->wantsdisconnect)
					{
						curr_truck->disconnectAutopilot();
					}
					//AIRPLANE KEYS
					float commandrate=4.0;
					//float dt=evt.timeSinceLastFrame;
					//turning
					if (curr_truck->replaymode)
					{
						if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FORWARD, 0.1f) && curr_truck->replaypos<=0)
						{
							curr_truck->replaypos++;
						}
						if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_BACKWARD, 0.1f) && curr_truck->replaypos > -curr_truck->replaylen)
						{
							curr_truck->replaypos--;
						}
						if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_FORWARD, 0.1f) && curr_truck->replaypos+10<=0)
						{
							curr_truck->replaypos+=10;
						}
						if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_BACKWARD, 0.1f) && curr_truck->replaypos-10 > -curr_truck->replaylen)
						{
							curr_truck->replaypos-=10;
						}
					} else
					{
						float tmp_left = RoR::Application::GetInputEngine()->getEventValue(EV_AIRPLANE_STEER_LEFT);
						float tmp_right = RoR::Application::GetInputEngine()->getEventValue(EV_AIRPLANE_STEER_RIGHT);
						float sum_steer = -tmp_left + tmp_right;
						RoR::Application::GetInputEngine()->smoothValue(curr_truck->aileron, sum_steer, dt*commandrate);
						curr_truck->hydrodircommand = curr_truck->aileron;
						curr_truck->hydroSpeedCoupling = !(RoR::Application::GetInputEngine()->isEventAnalog(EV_AIRPLANE_STEER_LEFT) && RoR::Application::GetInputEngine()->isEventAnalog(EV_AIRPLANE_STEER_RIGHT));
					}

					//pitch
					float tmp_pitch_up = RoR::Application::GetInputEngine()->getEventValue(EV_AIRPLANE_ELEVATOR_UP);
					float tmp_pitch_down = RoR::Application::GetInputEngine()->getEventValue(EV_AIRPLANE_ELEVATOR_DOWN);
					float sum_pitch = tmp_pitch_down - tmp_pitch_up;
					RoR::Application::GetInputEngine()->smoothValue(curr_truck->elevator, sum_pitch, dt*commandrate);

					//rudder
					float tmp_rudder_left = RoR::Application::GetInputEngine()->getEventValue(EV_AIRPLANE_RUDDER_LEFT);
					float tmp_rudder_right = RoR::Application::GetInputEngine()->getEventValue(EV_AIRPLANE_RUDDER_RIGHT);
					float sum_rudder = tmp_rudder_left - tmp_rudder_right;
					RoR::Application::GetInputEngine()->smoothValue(curr_truck->rudder, sum_rudder, dt*commandrate);

					//brake
					if (!curr_truck->replaymode && !curr_truck->parkingbrake)
					{
						curr_truck->brake=0.0;
						float brakevalue = RoR::Application::GetInputEngine()->getEventValue(EV_AIRPLANE_BRAKE);
						curr_truck->brake=curr_truck->brakeforce*0.66*brakevalue;
					}
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_PARKING_BRAKE))
					{
						curr_truck->parkingbrakeToggle();
						if (ow)
						{
							if (curr_truck->parkingbrake)
								OverlayManager::getSingleton().getOverlayElement("tracks/ap_brks_but")->setMaterialName("tracks/brks-on");
							else
								OverlayManager::getSingleton().getOverlayElement("tracks/ap_brks_but")->setMaterialName("tracks/brks-off");
						}
					}
					//reverse
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_REVERSE))
					{
						for (int i=0; i<curr_truck->free_aeroengine; i++)
							curr_truck->aeroengines[i]->toggleReverse();
					}

					// toggle engines
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_TOGGLE_ENGINES))
					{
						for (int i=0; i<curr_truck->free_aeroengine; i++)
							curr_truck->aeroengines[i]->flipStart();
					}

					//flaps
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_FLAPS_NONE))
					{
						if (curr_truck->flap>0)
							curr_truck->flap=0;
					}
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_FLAPS_FULL))
					{
						if (curr_truck->flap<5)
							curr_truck->flap=5;
					}
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_FLAPS_LESS))
					{
						if (curr_truck->flap>0)
							curr_truck->flap=(curr_truck->flap)-1;
					}
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_FLAPS_MORE))
					{
						if (curr_truck->flap<5)
							curr_truck->flap=(curr_truck->flap)+1;
					}

					//airbrakes
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_NONE))
					{
						if (curr_truck->airbrakeval>0)
							curr_truck->airbrakeval=0;
					}
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_FULL))
					{
						if (curr_truck->airbrakeval<5)
							curr_truck->airbrakeval=5;
					}
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_LESS))
					{
						if (curr_truck->airbrakeval>0)
							curr_truck->airbrakeval=(curr_truck->airbrakeval)-1;
					}
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_MORE))
					{
						if (curr_truck->airbrakeval<5)
							curr_truck->airbrakeval=(curr_truck->airbrakeval)+1;
					}

					//throttle
					float tmp_throttle = RoR::Application::GetInputEngine()->getEventBoolValue(EV_AIRPLANE_THROTTLE);
					if (tmp_throttle > 0)
					{
						for (int i=0; i<curr_truck->free_aeroengine; i++)
							curr_truck->aeroengines[i]->setThrottle(tmp_throttle);
					}
					if (RoR::Application::GetInputEngine()->isEventDefined(EV_AIRPLANE_THROTTLE_AXIS))
					{
						float f = RoR::Application::GetInputEngine()->getEventValue(EV_AIRPLANE_THROTTLE_AXIS);
						for (int i=0; i<curr_truck->free_aeroengine; i++)
							curr_truck->aeroengines[i]->setThrottle(f);
					}
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_DOWN, 0.1f))
					{
						//throttle down
						for (int i=0; i<curr_truck->free_aeroengine; i++)
							curr_truck->aeroengines[i]->setThrottle(curr_truck->aeroengines[i]->getThrottle()-0.05);
					}
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_UP, 0.1f))
					{
						//throttle up
						for (int i=0; i<curr_truck->free_aeroengine; i++)
							curr_truck->aeroengines[i]->setThrottle(curr_truck->aeroengines[i]->getThrottle()+0.05);
					}

					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_NO, 0.1f))
					{
						// no throttle
						for (int i=0; i<curr_truck->free_aeroengine; i++)
							curr_truck->aeroengines[i]->setThrottle(0);
					}
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_FULL, 0.1f))
					{
						// full throttle
						for (int i=0; i<curr_truck->free_aeroengine; i++)
							curr_truck->aeroengines[i]->setThrottle(1);
					}
					if (curr_truck->autopilot)
					{
						for (int i=0; i<curr_truck->free_aeroengine; i++)
							curr_truck->aeroengines[i]->setThrottle(curr_truck->autopilot->getThrottle(curr_truck->aeroengines[i]->getThrottle(), dt));
					}
				}
				if (curr_truck->driveable==BOAT)
				{
					//BOAT SPECIFICS

					//throttle
					if (RoR::Application::GetInputEngine()->isEventDefined(EV_BOAT_THROTTLE_AXIS))
					{
						float f = RoR::Application::GetInputEngine()->getEventValue(EV_BOAT_THROTTLE_AXIS);
						// use negative values also!
						f = f * 2 - 1;
						for (int i=0; i<curr_truck->free_screwprop; i++)
							curr_truck->screwprops[i]->setThrottle(-f);
					}
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_THROTTLE_DOWN, 0.1f))
					{
						//throttle down
						for (int i=0; i<curr_truck->free_screwprop; i++)
							curr_truck->screwprops[i]->setThrottle(curr_truck->screwprops[i]->getThrottle()-0.05);
					}
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_THROTTLE_UP, 0.1f))
					{
						//throttle up
						for (int i=0; i<curr_truck->free_screwprop; i++)
							curr_truck->screwprops[i]->setThrottle(curr_truck->screwprops[i]->getThrottle()+0.05);
					}


					// steer
					float tmp_steer_left = RoR::Application::GetInputEngine()->getEventValue(EV_BOAT_STEER_LEFT);
					float tmp_steer_right = RoR::Application::GetInputEngine()->getEventValue(EV_BOAT_STEER_RIGHT);
					float stime = RoR::Application::GetInputEngine()->getEventBounceTime(EV_BOAT_STEER_LEFT) + RoR::Application::GetInputEngine()->getEventBounceTime(EV_BOAT_STEER_RIGHT);
					float sum_steer = (tmp_steer_left - tmp_steer_right) * dt;
					// do not center the rudder!
					if (fabs(sum_steer)>0 && stime <= 0)
					{
						for (int i=0; i<curr_truck->free_screwprop; i++)
							curr_truck->screwprops[i]->setRudder(curr_truck->screwprops[i]->getRudder() + sum_steer);
					}
					if (RoR::Application::GetInputEngine()->isEventDefined(EV_BOAT_STEER_LEFT_AXIS) && RoR::Application::GetInputEngine()->isEventDefined(EV_BOAT_STEER_RIGHT_AXIS))
					{
						tmp_steer_left = RoR::Application::GetInputEngine()->getEventValue(EV_BOAT_STEER_LEFT_AXIS);
						tmp_steer_right = RoR::Application::GetInputEngine()->getEventValue(EV_BOAT_STEER_RIGHT_AXIS);
						sum_steer = (tmp_steer_left - tmp_steer_right);
						for (int i=0; i<curr_truck->free_screwprop; i++)
							curr_truck->screwprops[i]->setRudder(sum_steer);
					}
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_CENTER_RUDDER, 0.1f))
					{
						for (int i=0; i<curr_truck->free_screwprop; i++)
							curr_truck->screwprops[i]->setRudder(0);
					}

					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_REVERSE))
					{
						for (int i=0; i<curr_truck->free_screwprop; i++)
							curr_truck->screwprops[i]->toggleReverse();
					}
				}
				//COMMON KEYS

				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TRUCK_REMOVE))
				{
					BeamFactory::getSingleton().removeCurrentTruck();
				}
				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ROPELOCK))
				{
					curr_truck->ropeToggle(-1);
				}
				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_LOCK))
				{
					curr_truck->hookToggle(-1, HOOK_TOGGLE, -1);
					//SlideNodeLock
					curr_truck->toggleSlideNodeLock();
				}
				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_AUTOLOCK))
				{
					//unlock all autolocks
					curr_truck->hookToggle(-2, HOOK_UNLOCK, -1);
				}
				//strap
				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_SECURE_LOAD))
				{
					curr_truck->tieToggle(-1);
				}
				if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_COMMON_RESET_TRUCK) && !curr_truck->replaymode)
				{
					// stop any races
					stopTimer();
					// init
					curr_truck->reset();
				}
				if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_COMMON_REPAIR_TRUCK))
				{
#ifdef USE_OPENAL
					SoundScriptManager::getSingleton().trigOnce(curr_truck, SS_TRIG_REPAIR);
#endif //OPENAL
					curr_truck->reset(true);
				}
				//replay mode
				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_REPLAY_MODE))
				{
					stopTimer();
					curr_truck->setReplayMode(!curr_truck->replaymode);
				}

				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_CUSTOM_PARTICLES))
				{
					curr_truck->toggleCustomParticles();
				}

				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_SHOW_SKELETON))
				{
					if (curr_truck->skeleton)
						curr_truck->hideSkeleton(true);
					else
						curr_truck->showSkeleton(true, true);

					curr_truck->updateVisual();
				}

				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TRUCK_LIGHTS))
				{
					curr_truck->lightsToggle();
				}

				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TRUCK_BEACONS))
				{
					curr_truck->beaconsToggle();
				}

				//camera mode
				if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_COMMON_PRESSURE_LESS) && curr_truck)
				{
					if (ow) ow->showPressureOverlay(true);
#ifdef USE_OPENAL
					SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_AIR);
#endif // OPENAL
					curr_truck->addPressure(-dt*10.0);
					pressure_pressed=true;
				} else if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_COMMON_PRESSURE_MORE))
				{
					if (ow) ow->showPressureOverlay(true);
#ifdef USE_OPENAL
					SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_AIR);
#endif // OPENAL
					curr_truck->addPressure(dt*10.0);
					pressure_pressed=true;
				} else if (pressure_pressed)
				{
#ifdef USE_OPENAL
					SoundScriptManager::getSingleton().trigStop(curr_truck, SS_TRIG_AIR);
#endif // OPENAL
					pressure_pressed=false;
					if (ow) ow->showPressureOverlay(false);
				}

				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESCUE_TRUCK, 0.5f) && !gEnv->network && curr_truck->driveable != AIRPLANE)
				{
					if (!BeamFactory::getSingleton().enterRescueTruck())
					{
#ifdef USE_MYGUI
						RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("No rescue truck found!"), "warning.png");
#endif // USE_MYGUI
					}
				}

				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_BLINK_LEFT))
				{
					if (curr_truck->getBlinkType() == BLINK_LEFT)
						curr_truck->setBlinkType(BLINK_NONE);
					else
						curr_truck->setBlinkType(BLINK_LEFT);
				}

				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_BLINK_RIGHT))
				{
					if (curr_truck->getBlinkType() == BLINK_RIGHT)
						curr_truck->setBlinkType(BLINK_NONE);
					else
						curr_truck->setBlinkType(BLINK_RIGHT);
				}

				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_BLINK_WARN))
				{
					if (curr_truck->getBlinkType() == BLINK_WARN)
						curr_truck->setBlinkType(BLINK_NONE);
					else
						curr_truck->setBlinkType(BLINK_WARN);
				}

			} // end of truck!=-1
		}

#ifdef USE_CAELUM
		if (SSETTING("Sky effects", "Caelum (best looking, slower)") == "Caelum (best looking, slower)")
		{
			Real time_factor = 1000.0f;
			Real multiplier = 10;
			bool update_time = false;

			if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_CAELUM_INCREASE_TIME) && gEnv->terrainManager->getSkyManager())
			{
				update_time = true;
			}
			else if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_CAELUM_INCREASE_TIME_FAST) && gEnv->terrainManager->getSkyManager())
			{
				time_factor *= multiplier;
				update_time = true;
			}
			else if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_CAELUM_DECREASE_TIME) && gEnv->terrainManager->getSkyManager())
			{
				time_factor = -time_factor;
				update_time = true;
			}
			else if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_CAELUM_DECREASE_TIME_FAST) && gEnv->terrainManager->getSkyManager())
			{
				time_factor *= -multiplier;
				update_time = true;
			}
			else
			{
				time_factor = 1.0f;
				update_time = gEnv->terrainManager->getSkyManager()->getTimeFactor() != 1.0f;
			}

			if ( update_time )
			{
				gEnv->terrainManager->getSkyManager()->setTimeFactor(time_factor);
#ifdef USE_MYGUI
				RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Time set to ") + gEnv->terrainManager->getSkyManager()->getPrettyTime(), "weather_sun.png", 1000);
#endif // USE_MYGUI
			}
		}
#endif // USE_CAELUM

		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_RENDER_MODE, 0.5f))
		{
			static int mSceneDetailIndex;
			mSceneDetailIndex = (mSceneDetailIndex + 1) % 3;
			switch (mSceneDetailIndex)
			{
			case 0:
				gEnv->mainCamera->setPolygonMode(PM_SOLID);
				break;
			case 1:
				gEnv->mainCamera->setPolygonMode(PM_WIREFRAME);
				break;
			case 2:
				gEnv->mainCamera->setPolygonMode(PM_POINTS);
				break;
			}
		}
		
		if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_COMMON_ENTER_OR_EXIT_TRUCK) && mTimeUntilNextToggle <= 0)
		{
			mTimeUntilNextToggle = 0.5; // Some delay before trying to re-enter(exit) truck
			// perso in/out
			int current_truck = BeamFactory::getSingleton().getCurrentTruckNumber();
			int free_truck    = BeamFactory::getSingleton().getTruckCount();
			Beam **trucks     = BeamFactory::getSingleton().getTrucks();
			if (current_truck == -1)
			{
				// find the nearest truck
				float mindist   =  1000.0;
				int   minindex  = -1;
				for (int i=0; i<free_truck; i++)
				{
					if (!trucks[i]) continue;
					if (!trucks[i]->driveable) continue;
					if (trucks[i]->cinecameranodepos[0] == -1)
					{
						LOG("cinecam missing, cannot enter truck!");
						continue;
					}
					float len = 0.0f;
					if (gEnv->player)
					{
						len = trucks[i]->nodes[trucks[i]->cinecameranodepos[0]].AbsPosition.distance(gEnv->player->getPosition() + Vector3(0.0, 2.0, 0.0));
					}
					if (len < mindist)
					{
						mindist = len;
						minindex = i;
					}
				}
				if (mindist < 20.0)
				{
					BeamFactory::getSingleton().setCurrentTruck(minindex);
				}
			} else if (curr_truck->nodes[0].Velocity.length() < 1.0f)
			{
				BeamFactory::getSingleton().setCurrentTruck(-1);
			} else
			{
				curr_truck->brake    = curr_truck->brakeforce * 0.66;
				mTimeUntilNextToggle = 0.0; // No delay in this case: the truck must brake like braking normally
			}
		}
	} else
	{
		//no terrain or truck loaded

#ifdef USE_MYGUI
		if (SelectorWindow::getSingleton().isFinishedSelecting())
		{
			if (loading_state==TERRAIN_LOADED)
			{
				CacheEntry *selection = SelectorWindow::getSingleton().getSelection();
				Skin *skin = SelectorWindow::getSingleton().getSelectedSkin();
				std::vector<String> config = SelectorWindow::getSingleton().getTruckConfig();
				std::vector<String> *configptr = &config;
				if (config.size() == 0) configptr = 0;
				if (selection)
					initTrucks(true, selection->fname, selection->fext, configptr, false, skin);
				else
					initTrucks(false, "");

			} 
			else if (loading_state == RELOADING)
			{
				CacheEntry *selection = SelectorWindow::getSingleton().getSelection();
				Skin *skin = SelectorWindow::getSingleton().getSelectedSkin();
				Beam *local_truck = nullptr;
				if (selection != nullptr)
				{
					/* We load an extra truck */
					std::vector<String> *config_ptr = nullptr;
					std::vector<String> config = SelectorWindow::getSingleton().getTruckConfig();
					if (config.size() > 0)
					{
						config_ptr = & config;
					}

					local_truck = BeamFactory::getSingleton().createLocal(reload_pos, reload_dir, selection->fname, reload_box, false, flaresMode, config_ptr, skin, freeTruckPosition);
					freeTruckPosition = false; // reset this, only to be used once
				}

				if (gEnv->surveyMap && local_truck)
				{
					SurveyMapEntity *e = gEnv->surveyMap->createNamedMapEntity("Truck"+TOSTRING(local_truck->trucknum), SurveyMapManager::getTypeByDriveable(local_truck->driveable));
					if (e)
					{
						e->setState(DESACTIVATED);
						e->setVisibility(true);
						e->setPosition(reload_pos);
						e->setRotation(-Radian(local_truck->getHeadingDirectionAngle()));
					}
				}

				SelectorWindow::getSingleton().hide();
				loading_state = ALL_LOADED;

				RoR::Application::GetGuiManager()->unfocus();

				if (local_truck != nullptr && local_truck->driveable != NOT_DRIVEABLE)
				{
					/* We are supposed to be in this truck, if it is a truck */
					if (local_truck->engine != nullptr)
					{
						local_truck->engine->start();
					}
					BeamFactory::getSingleton().setCurrentTruck(local_truck->trucknum);
				} 
				else if (gEnv->player != nullptr)
				{
					// if it is a load or trailer, then stay in player mode
					// but relocate to the new position, so we don't spawn the dialog again
					gEnv->player->move(Vector3(3.0, 0.2, 0.0));
				}
			}
		}
#endif //MYGUI
	}



	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TRUCK_INFO) && curr_truck)
	{
		mTruckInfoOn = ! mTruckInfoOn;
		dirty=true;
		if (ow) ow->truckhud->show(mTruckInfoOn);
	}

	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_HIDE_GUI))
	{
		hidegui = !hidegui;
		hideGUI(hidegui);
		dirty=true;
	}

	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_STATS) && (loading_state == ALL_LOADED || loading_state == TERRAIN_EDITOR))
	{
		dirty=true;
		if (mStatsOn==0)
			mStatsOn=1;
		else if (mStatsOn==1)
			mStatsOn=0;
		else if (mStatsOn==2)
			mStatsOn=0;

		if (ow) ow->showDebugOverlay(mStatsOn);
	}

	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_MAT_DEBUG))
	{
		if (mStatsOn==0)
			mStatsOn=2;
		else if (mStatsOn==1)
			mStatsOn=2;
		else if (mStatsOn==2)
			mStatsOn=0;
		dirty=true;
		if (ow) ow->showDebugOverlay(mStatsOn);
	}

	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_OUTPUT_POSITION) && loading_state == ALL_LOADED)
	{
		Vector3 position(Vector3::ZERO);
		Radian rotation(0);
		if (BeamFactory::getSingleton().getCurrentTruckNumber() == -1)
		{
			if (gEnv->player)
			{
				position = gEnv->player->getPosition();
				rotation = gEnv->player->getRotation() + Radian(Math::PI);
			}
		} else
		{
			position = curr_truck->getPosition();
			Vector3 idir = curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition - curr_truck->nodes[curr_truck->cameranodedir[0]].RelPosition;
			rotation = atan2(idir.dotProduct(Vector3::UNIT_X), idir.dotProduct(-Vector3::UNIT_Z));
		}
		LOG("Position: " + TOSTRING(position.x) + ", "+ TOSTRING(position.y) + ", " + TOSTRING(position.z) + ", 0, " + TOSTRING(rotation.valueDegrees()) + ", 0");
	}

	//update window
	if (!gEnv->renderWindow->isAutoUpdated())
	{
		if (dirty)
			gEnv->renderWindow->update();
		else
			sleepMilliSeconds(10);
	}
	// Return true to continue rendering
	return true;
}

void RoRFrameListener::shutdown_final()
{
	LOG(" ** Shutdown preparation");
	
	loading_state = EXITING;

	//RoR::Application::GetGuiManager()->shutdown();

#ifdef USE_SOCKETW
	if (gEnv->network) gEnv->network->disconnect();
#endif //SOCKETW
	
#ifdef USE_OPENAL
	SoundScriptManager::getSingleton().setEnabled(false);
#endif //OPENAL

	LOG(" ** Shutdown final");

	if (gEnv && gEnv->terrainManager)
	{
		if (gEnv->terrainManager->getWater()) gEnv->terrainManager->getWater()->prepareShutdown();
		if (gEnv->terrainManager->getEnvmap()) gEnv->terrainManager->getEnvmap()->prepareShutdown();
	}
	if (dashboard) dashboard->prepareShutdown();
	if (heathaze) heathaze->prepareShutdown();

	BeamFactory::getSingleton().prepareShutdown();

	RoR::Application::GetInputEngine()->prepareShutdown();

	// RoRFrameListener::shutdown_final() is allways called by main thread.
	// Therefore we need no syncing here.
	m_main_thread_control->RequestShutdown();

	shutdownall = true;
}

void RoRFrameListener::hideMap()
{
#ifdef USE_MYGUI
	if (gEnv->surveyMap) gEnv->surveyMap->setVisibility(false);
#endif //USE_MYGUI
}



void RoRFrameListener::initTrucks(bool loadmanual, Ogre::String selected, Ogre::String selectedExtension /* = "" */, const std::vector<Ogre::String> *truckconfig /* = 0 */, bool enterTruck /* = false */, Skin *skin /* = NULL */)
{
	//we load truck
	char *selectedchr = const_cast< char *> (selected.c_str());

	if (loadmanual)
	{
		Beam *b = 0;
		Vector3 spawnpos = gEnv->terrainManager->getSpawnPos();
		Quaternion spawnrot = Quaternion::ZERO;

		b = BeamFactory::getSingleton().createLocal(spawnpos, spawnrot, selectedchr, 0, false, flaresMode, truckconfig, skin);

		if (enterTruck)
		{
			if (b)
				BeamFactory::getSingleton().setCurrentTruck(b->trucknum);
			else
				BeamFactory::getSingleton().setCurrentTruck(-1);
		}

#ifdef USE_MYGUI
		if (b && gEnv->surveyMap)
		{
			SurveyMapEntity *e = gEnv->surveyMap->createNamedMapEntity("Truck"+TOSTRING(b->trucknum), SurveyMapManager::getTypeByDriveable(b->driveable));
			if (e)
			{
				e->setState(DESACTIVATED);
				e->setVisibility(true);
				e->setPosition(spawnpos.x, spawnpos.z);
				e->setRotation(-Radian(b->getHeadingDirectionAngle()));
			}
		}
#endif //USE_MYGUI

		if (b && b->engine)
		{
			b->engine->start();
		}
	}
	
	gEnv->terrainManager->loadPreloadedTrucks();

	LOG("EFL: beam instanciated");

	if (!enterTruck)
	{
		BeamFactory::getSingleton().setCurrentTruck(-1);
	}

	// fix for problem on loading
	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();

	if (enterTruck && curr_truck && curr_truck->free_node == 0)
	{
		BeamFactory::getSingleton().setCurrentTruck(-1);
	}

	//force perso start
	if (persostart != Vector3(0,0,0) && gEnv->player)
	{
		gEnv->player->setPosition(persostart);
	}

	loading_state = ALL_LOADED;

	if (ISETTING("OutGauge Mode", 0) > 0)
	{
		new OutProtocol();
	}

	LOG("initTrucks done");

#ifdef USE_MYGUI
	RoR::Application::GetGuiManager()->unfocus();
#endif //USE_MYGUI
}

void RoRFrameListener::changedCurrentTruck(Beam *previousTruck, Beam *currentTruck)
{
	// hide any old dashes
	if (previousTruck && previousTruck->dash)
	{
		previousTruck->dash->setVisible3d(false);
	}
	// show new
	if (currentTruck && currentTruck->dash)
	{
		currentTruck->dash->setVisible3d(true);
	}
	
	// normal workflow
	if (!currentTruck)
	{
		// get player out of the vehicle
		if (previousTruck && gEnv->player)
		{
			// detach from truck
			gEnv->player->setBeamCoupling(false);
			// update position
			gEnv->player->setPosition(previousTruck->getPosition());
			// update rotation
			gEnv->player->updateCharacterRotation();
		}

		//force feedback
		if (forcefeedback) forcefeedback->setEnabled(false);



		// hide truckhud
		if (ow) ow->truckhud->show(false);

		//getting outside
		Vector3 position = Vector3::ZERO;
		if (previousTruck)
		{
			previousTruck->prepareInside(false);

			if (previousTruck->dash)
				previousTruck->dash->setVisible(false);

			// this workaround enables trucks to spawn that have no cinecam. required for cmdline options
			if (previousTruck->cinecameranodepos[0] != -1 && previousTruck->cameranodepos[0] != -1 && previousTruck->cameranoderoll[0] != -1)
			{
				// truck has a cinecam
				position=previousTruck->nodes[previousTruck->cinecameranodepos[0]].AbsPosition;
				position+=-2.0*((previousTruck->nodes[previousTruck->cameranodepos[0]].RelPosition-previousTruck->nodes[previousTruck->cameranoderoll[0]].RelPosition).normalisedCopy());
				position+=Vector3(0.0, -1.0, 0.0);
			} else
			{
				// truck has no cinecam
				position=previousTruck->nodes[0].AbsPosition;
			}
		}
		//position.y=hfinder->getHeightAt(position.x,position.z);
		if (gEnv->player && position != Vector3::ZERO)
		{
			gEnv->player->setPosition(position);
			gEnv->player->updateCharacterRotation();
			//gEnv->player->setVisible(true);
		}
		if (ow) ow->showDashboardOverlays(false, currentTruck);
#ifdef USE_OPENAL
		SoundScriptManager::getSingleton().trigStop(previousTruck, SS_TRIG_AIR);
		SoundScriptManager::getSingleton().trigStop(previousTruck, SS_TRIG_PUMP);
#endif // OPENAL

		if (!BeamFactory::getSingleton().allTrucksForcedActive())
		{
			int free_truck = BeamFactory::getSingleton().getTruckCount();
			Beam **trucks =  BeamFactory::getSingleton().getTrucks();

			for (int t = 0; t < free_truck; t++)
			{
				if (!trucks[t]) continue;
				trucks[t]->sleepcount = 9;
			} // make trucks synchronous
		}

		TRIGGER_EVENT(SE_TRUCK_EXIT, previousTruck?previousTruck->trucknum:-1);
	} else
	{
		//getting inside
		currentTruck->desactivate();

		if (ow &&!hidegui)
		{
			ow->showDashboardOverlays(true, currentTruck);
		}

		currentTruck->activate();
		//if (trucks[current_truck]->engine->running) trucks[current_truck]->audio->playStart();
		//hide unused items
		if (ow && currentTruck->free_active_shock==0)
			(OverlayManager::getSingleton().getOverlayElement("tracks/rollcorneedle"))->hide();
		//					rollcorr_node->setVisible((trucks[current_truck]->free_active_shock>0));
		//help panel
		//force feedback
		if (forcefeedback) forcefeedback->setEnabled(currentTruck->driveable==TRUCK); //only for trucks so far



		// attach player to truck
		if (gEnv->player)
		{
			gEnv->player->setBeamCoupling(true, currentTruck);
		}

		if (ow)
		{
			try
			{
				// we wont crash for help panels ...
				if (currentTruck->hashelp)
				{
					OverlayManager::getSingleton().getOverlayElement("tracks/helppanel")->setMaterialName(currentTruck->helpmat);
					OverlayManager::getSingleton().getOverlayElement("tracks/machinehelppanel")->setMaterialName(currentTruck->helpmat);
				}
				else
				{
					OverlayManager::getSingleton().getOverlayElement("tracks/helppanel")->setMaterialName("tracks/black");
					OverlayManager::getSingleton().getOverlayElement("tracks/machinehelppanel")->setMaterialName("tracks/black");
				}
			} catch(...)
			{
			}
			// enable gui mods
			if (currentTruck->speedomat != String(""))
				OverlayManager::getSingleton().getOverlayElement("tracks/speedo")->setMaterialName(currentTruck->speedomat);
			else
				OverlayManager::getSingleton().getOverlayElement("tracks/speedo")->setMaterialName("tracks/Speedo");

			if (currentTruck->tachomat != String(""))
				OverlayManager::getSingleton().getOverlayElement("tracks/tacho")->setMaterialName(currentTruck->tachomat);
			else
				OverlayManager::getSingleton().getOverlayElement("tracks/tacho")->setMaterialName("tracks/Tacho");
		}
		
		TRIGGER_EVENT(SE_TRUCK_ENTER, currentTruck?currentTruck->trucknum:-1);
	}
}


bool RoRFrameListener::updateTruckMirrors(float dt)
{
	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();
	if (!curr_truck) return false;

	for (std::vector<VideoCamera *>::iterator it=curr_truck->vidcams.begin(); it!=curr_truck->vidcams.end(); it++)
	{
		(*it)->update(dt);
	}
	return true;
}

// Override frameStarted event to process that (don't care about frameEnded)
bool RoRFrameListener::frameStarted(const FrameEvent& evt)
{
	if (shutdownall) // shortcut: press ESC in credits
	{
		m_main_thread_control->Exit();
		return false;
	}

	float dt=evt.timeSinceLastFrame;
	if (dt==0) return true;
	if (dt>1.0/20.0) dt=1.0/20.0;
	rtime+=dt; //real time
	if (gEnv->renderWindow->isClosed())
	{
		return false;
	}

	// update GUI
	RoR::Application::GetInputEngine()->Capture();

	//if (gEnv->collisions) 	printf("> ground model used: %s\n", gEnv->collisions->last_used_ground_model->name);

	// exit frame started method when just displaying the GUI
#ifdef USE_MYGUI
	if (LoadingWindow::getSingleton().getFrameForced())
	{
		return true;
	}
#endif //MYGUI

	// update OutProtocol
	if (OutProtocol::getSingletonPtr())
	{
		OutProtocol::getSingleton().update(dt);
	}
	// the truck we use got deleted D:
	//if (current_truck != -1 && trucks[current_truck] == 0)
	//	BeamFactory::getSingleton().setCurrentTruck(-1);

	// update network gui if required, at most every 2 seconds
	if (gEnv->network)
	{
		netcheckGUITimer += dt;
		if (netcheckGUITimer > 2)
		{
			checkRemoteStreamResultsChanged();
			netcheckGUITimer=0;
		}

#ifdef USE_SOCKETW
#ifdef USE_MYGUI
		// update net quality icon
		if (gEnv->network->getNetQualityChanged())
		{
			GUI_Multiplayer::getSingleton().update();
		}
#endif // USE_MYGUI
#endif // USE_SOCKETW

		// now update mumble 3d audio things
#ifdef USE_MUMBLE
		if (gEnv->player)
		{
			MumbleIntegration::getSingleton().update(gEnv->mainCamera->getPosition(), gEnv->player->getPosition() + Vector3(0, 1.8f, 0));
		}
#endif // USE_MUMBLE
	}

	if (gEnv->cameraManager && (loading_state == ALL_LOADED || loading_state == TERRAIN_EDITOR))
	{
		gEnv->cameraManager->update(dt);
	}

	if (gEnv->surveyMap && (loading_state == ALL_LOADED || loading_state == TERRAIN_EDITOR))
	{
		gEnv->surveyMap->update(dt);
	}

	// update mirrors after moving the camera as we use the camera position in mirrors
	updateTruckMirrors(dt);

#ifdef USE_OPENAL
	// update audio listener position
	static Vector3 lastCameraPosition;
	Vector3 cameraSpeed = (gEnv->mainCamera->getPosition() - lastCameraPosition) / dt;
	lastCameraPosition = gEnv->mainCamera->getPosition();

	if(loading_state == ALL_LOADED)
		SoundScriptManager::getSingleton().setCamera(gEnv->mainCamera->getPosition(), gEnv->mainCamera->getDirection(), gEnv->mainCamera->getUp(), cameraSpeed);
#endif // USE_OPENAL
	
	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();

	// terrain updates
	if (gEnv->terrainManager)
	{
		// update animated objects
		gEnv->terrainManager->update(dt);

		// env map update
		if (gEnv->terrainManager->getEnvmap())
		{
			if (curr_truck)
			{
				gEnv->terrainManager->getEnvmap()->update(curr_truck->getPosition(), curr_truck);
			} else
			{
				float height = gEnv->terrainManager->getMaxTerrainSize().y;
				if (gEnv->terrainManager->getHeightFinder())
				{
					height = gEnv->terrainManager->getHeightFinder()->getHeightAt(terrainxsize / 2.0f, terrainzsize / 2.0f );
				}
				gEnv->terrainManager->getEnvmap()->update(Vector3(terrainxsize / 2.0f, height + 50.0f, terrainzsize / 2.0f));
			}
		}

		// water
		if (loading_state==ALL_LOADED || loading_state == TERRAIN_EDITOR)
		{
			IWater *water = gEnv->terrainManager->getWater();
			if (water)
			{
				water->setCamera(gEnv->mainCamera);
				if (curr_truck)
				{
					water->moveTo(water->getHeightWaves(curr_truck->getPosition()));
				} else
				{
					water->moveTo(water->getHeight());
				}
				water->framestep(dt);
			}
		}

		// trigger updating of shadows etc
		SkyManager *sky = gEnv->terrainManager->getSkyManager();
		if(sky) sky->detectUpdate();
		
		gEnv->terrainManager->update(dt);
	}

	//update visual - antishaking
	if (loading_state==ALL_LOADED || loading_state == TERRAIN_EDITOR)
	{
		BeamFactory::getSingleton().updateVisual(dt);

		// add some example AI
		//if (loadedTerrain == "simple.terrn2")
			//BeamFactory::getSingleton().updateAI(dt);
	}


	if (!updateEvents(dt))
	{
		LOG("exiting...");
		return false;
	}

	// update gui 3d arrow
	if (ow && dirvisible && loading_state==ALL_LOADED)
	{
		ow->UpdateDirectionArrow(curr_truck, dirArrowPointed);
	}

	// one of the input modes is immediate, so setup what is needed for immediate mouse/key movement
	if (mTimeUntilNextToggle >= 0)
	{
		mTimeUntilNextToggle -= dt;
	}

	// one of the input modes is immediate, so update the movement vector
	if (loading_state == ALL_LOADED || loading_state == TERRAIN_EDITOR)
	{
		BeamFactory::getSingleton().checkSleepingState();

		// we simulate one truck, it will take care of the others (except networked ones)
		BeamFactory::getSingleton().calcPhysics(dt);

		updateIO(dt);

		//// updateGUI(dt); (refactored into pieces)

		if (ow != nullptr)
		{

#ifdef USE_MYGUI

			Beam** vehicles  = BeamFactory::getSingleton().getTrucks();
			int num_vehicles = BeamFactory::getSingleton().getTruckCount();

			// Update survey map
			if (gEnv->surveyMap != nullptr && gEnv->surveyMap->getVisibility())
			{
				gEnv->surveyMap->Update(vehicles, num_vehicles);
			}

#endif // USE_MYGUI

			Beam* vehicle = BeamFactory::getSingleton().getCurrentTruck();
			if (vehicle != nullptr)
			{
				//update the truck info gui (also if not displayed!)
				ow->truckhud->update(dt, vehicle, mTruckInfoOn);

#ifdef FEAT_TIMING
				BES.updateGUI(dt);
#endif // FEAT_TIMING

#ifdef USE_MYGUI
				// update mouse picking lines, etc
				RoR::Application::GetSceneMouse()->update(dt);
#endif //USE_MYGUI

				if (pressure_pressed)
				{
					ow->UpdatePressureTexture(vehicle->getPressure());
				}

				if (raceStartTime > 0)
				{
					updateRacingGUI(); // TODO: Move to OverlayWrapper
				}

				if (vehicle->driveable == TRUCK && vehicle->engine != nullptr)
				{
					ow->UpdateLandVehicleHUD(vehicle, flipflop);
				}
				else if (vehicle->driveable == AIRPLANE)
				{
					ow->UpdateAerialHUD(vehicle);
				}
			}
		}
	}


	// TODO: check if all wheels are on a certain event id
	// wheels[nodes[i].wheelid].lastEventHandler

#ifdef USE_ANGELSCRIPT
	ScriptEngine::getSingleton().framestep(dt);
#endif

	// update network labels
	if (gEnv->network)
	{
		CharacterFactory::getSingleton().updateLabels();
	}

	return true;
}

bool RoRFrameListener::frameEnded(const FrameEvent& evt)
{
	// TODO: IMPROVE STATS
	if (ow && mStatsOn) ow->updateStats();

	//		moveCamera();

	// workaround to be able to show a single waiting sign before working on the files
	//if (uiloader && uiloader->hasWork())
	//	uiloader->dowork();

	if (heathaze)
	{
		heathaze->update();
	}

#ifdef USE_SOCKETW
	if (gEnv->network)
	{
		// process all packets and streams received
		NetworkStreamManager::getSingleton().update();
	}
#endif //SOCKETW

	return true;
}

void RoRFrameListener::showLoad(int type, const Ogre::String &instance, const Ogre::String &box)
{
	int free_truck    = BeamFactory::getSingleton().getTruckCount();
	Beam **trucks     = BeamFactory::getSingleton().getTrucks();

	// first, test if the place if clear, BUT NOT IN MULTIPLAYER
	if (!gEnv->network)
	{
		collision_box_t *spawnbox = gEnv->collisions->getBox(instance, box);
		for (int t=0; t < free_truck; t++)
		{
			if (!trucks[t]) continue;
			for (int i=0; i < trucks[t]->free_node; i++)
			{
				if (gEnv->collisions->isInside(trucks[t]->nodes[i].AbsPosition, spawnbox))
				{
#ifdef USE_MYGUI
					RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Please clear the place first"), "error.png");
#endif // USE_MYGUI
					gEnv->collisions->clearEventCache();
					return;
				}
			}
		}
	}
	reload_pos = gEnv->collisions->getPosition(instance, box);
	reload_dir = gEnv->collisions->getDirection(instance, box);
	reload_box = gEnv->collisions->getBox(instance, box);
	loading_state = RELOADING;
	hideMap();
#ifdef USE_MYGUI
	SelectorWindow::getSingleton().show(SelectorWindow::LoaderType(type));
#endif //USE_MYGUI
}

void RoRFrameListener::setDirectionArrow(char *text, Vector3 position)
{
	if (ow == nullptr) return;

	if (text == nullptr)
	{
		ow->HideDirectionOverlay();
		dirvisible = false;
		dirArrowPointed = Vector3::ZERO;
	}
	else
	{
		ow->ShowDirectionOverlay(text);
		dirvisible = true;
		dirArrowPointed = position;
	}

}

void RoRFrameListener::netDisconnectTruck(int number)
{
	// we will remove the truck completely
	// TODO: fix that below!
	//removeTruck(number);
#ifdef USE_MYGUI
	if (gEnv->surveyMap)
	{
		SurveyMapEntity *e = gEnv->surveyMap->getMapEntityByName("Truck"+TOSTRING(number));
		if (e)
			e->setVisibility(false);
	}
#endif //USE_MYGUI
}

/* --- Window Events ------------------------------------------ */
void RoRFrameListener::windowResized(Ogre::RenderWindow* rw)
{
	if (!rw) return;
	LOG("*** windowResized");

	// Update mouse screen width/height
	unsigned int width, height, depth;
	int left, top;
	rw->getMetrics(width, height, depth, left, top);

	if (ow) ow->windowResized();
	if (gEnv->surveyMap) gEnv->surveyMap->windowResized();

	//update mouse area
	RoR::Application::GetInputEngine()->windowResized(rw);
}

//Unattach OIS before window shutdown (very important under Linux)
void RoRFrameListener::windowClosed(Ogre::RenderWindow* rw)
{
	LOG("*** windowClosed");
}

void RoRFrameListener::windowMoved(Ogre::RenderWindow* rw)
{
	LOG("*** windowMoved");
}

void RoRFrameListener::windowFocusChange(Ogre::RenderWindow* rw)
{
	LOG("*** windowFocusChange");
	RoR::Application::GetInputEngine()->resetKeys();
}

void RoRFrameListener::pauseSim(bool value)
{
	// TODO: implement this (how to do so?)
	static int savedmode = -1;
	if (value && loading_state == PAUSE)
		// already paused
		return;
	if (value)
	{
		savedmode = loading_state;
		loading_state = PAUSE;
		LOG("** pausing game");
	} else if (!value && savedmode != -1)
	{
		loading_state = savedmode;
		LOG("** unpausing game");
	}
}

void RoRFrameListener::hideGUI(bool visible)
{
#ifdef USE_MYGUI
	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();
	Console *c = RoR::Application::GetConsole();
	if (c) c->setVisible(!visible);

	if (visible)
	{
		if (ow) ow->showDashboardOverlays(false, curr_truck);
		if (ow) ow->truckhud->show(false);
		if (gEnv->surveyMap) gEnv->surveyMap->setVisibility(false);
#ifdef USE_SOCKETW
		if (gEnv->network) GUI_Multiplayer::getSingleton().setVisible(false);
#endif // USE_SOCKETW
	} else
	{
		if (curr_truck
			&& gEnv->cameraManager
			&& gEnv->cameraManager->hasActiveBehavior()
			&& gEnv->cameraManager->getCurrentBehavior() != CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM)
		{
			if (ow) ow->showDashboardOverlays(true, curr_truck);
		}
#ifdef USE_SOCKETW
		if (gEnv->network) GUI_Multiplayer::getSingleton().setVisible(true);
#endif // USE_SOCKETW
	}
#endif // USE_MYGUI
}

// show/hide all particle systems
void RoRFrameListener::showspray(bool s)
{
	DustManager::getSingleton().setVisible(s);
}

void RoRFrameListener::setLoadingState(int value)
{
	loading_state = value;
}

void RoRFrameListener::setNetPointToUID(int uid)
{
	// TODO: setup arrow
	netPointToUID = uid;
}


void RoRFrameListener::checkRemoteStreamResultsChanged()
{
#ifdef USE_MYGUI
#ifdef USE_SOCKETW
	if (BeamFactory::getSingleton().checkStreamsResultsChanged())
		GUI_Multiplayer::getSingleton().update();
#endif // USE_SOCKETW
#endif // USE_MYGUI
}




bool RoRFrameListener::RTSSgenerateShadersForMaterial(String curMaterialName, String normalTextureName)
{
	if (!BSETTING("Use RTShader System", false)) return false;
	bool success;

	// Create the shader based technique of this material.
	success = RTShader::ShaderGenerator::getSingleton().createShaderBasedTechnique(curMaterialName,
			 			MaterialManager::DEFAULT_SCHEME_NAME,
			 			RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
	if (!success)
		return false;

	// Setup custom shader sub render states according to current setup.
	MaterialPtr curMaterial = MaterialManager::getSingleton().getByName(curMaterialName);


	// Grab the first pass render state.
	// NOTE: For more complicated samples iterate over the passes and build each one of them as desired.
	RTShader::RenderState* renderState = RTShader::ShaderGenerator::getSingleton().getRenderState(RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME, curMaterialName, 0);

	// Remove all sub render states.
	renderState->reset();


#ifdef RTSHADER_SYSTEM_BUILD_CORE_SHADERS
	// simple vertex lightning
	/*
	{
		RTShader::SubRenderState* perPerVertexLightModel = RTShader::ShaderGenerator::getSingleton().createSubRenderState(RTShader::FFPLighting::Type);
		renderState->addTemplateSubRenderState(perPerVertexLightModel);
	}
	*/
#endif
#ifdef RTSHADER_SYSTEM_BUILD_EXT_SHADERS
	if (normalTextureName.empty())
	{
		// SSLM_PerPixelLighting
		RTShader::SubRenderState* perPixelLightModel = RTShader::ShaderGenerator::getSingleton().createSubRenderState(RTShader::PerPixelLighting::Type);

		renderState->addTemplateSubRenderState(perPixelLightModel);
	} else
	{
		// SSLM_NormalMapLightingTangentSpace
		RTShader::SubRenderState* subRenderState = RTShader::ShaderGenerator::getSingleton().createSubRenderState(RTShader::NormalMapLighting::Type);
		RTShader::NormalMapLighting* normalMapSubRS = static_cast<RTShader::NormalMapLighting*>(subRenderState);

		normalMapSubRS->setNormalMapSpace(RTShader::NormalMapLighting::NMS_TANGENT);
		normalMapSubRS->setNormalMapTextureName(normalTextureName);

		renderState->addTemplateSubRenderState(normalMapSubRS);
	}
	// SSLM_NormalMapLightingObjectSpace
	/*
	{
		// Apply normal map only on main entity.
		if (entity->getName() == MAIN_ENTITY_NAME)
		{
			RTShader::SubRenderState* subRenderState = mShaderGenerator->createSubRenderState(RTShader::NormalMapLighting::Type);
			RTShader::NormalMapLighting* normalMapSubRS = static_cast<RTShader::NormalMapLighting*>(subRenderState);

			normalMapSubRS->setNormalMapSpace(RTShader::NormalMapLighting::NMS_OBJECT);
			normalMapSubRS->setNormalMapTextureName("Panels_Normal_Obj.png");

			renderState->addTemplateSubRenderState(normalMapSubRS);
		}

		// It is secondary entity -> use simple per pixel lighting.
		else
		{
			RTShader::SubRenderState* perPixelLightModel = mShaderGenerator->createSubRenderState(RTShader::PerPixelLighting::Type);
			renderState->addTemplateSubRenderState(perPixelLightModel);
		}
	} */

#endif

	// Invalidate this material in order to re-generate its shaders.
	RTShader::ShaderGenerator::getSingleton().invalidateMaterial(RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME, curMaterialName);
	return true;
}

void RoRFrameListener::RTSSgenerateShaders(Entity* entity, String normalTextureName)
{
	if (!BSETTING("Use RTShader System", false)) return;
	for (unsigned int i=0; i < entity->getNumSubEntities(); ++i)
	{
		SubEntity* curSubEntity = entity->getSubEntity(i);
		const String& curMaterialName = curSubEntity->getMaterialName();

		RTSSgenerateShadersForMaterial(curMaterialName, normalTextureName);
	}
}

void RoRFrameListener::reloadCurrentTruck()
{
	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();
	if (!curr_truck) return;

	// try to load the same truck again
	Beam *newBeam = BeamFactory::getSingleton().createLocal(reload_pos, reload_dir, curr_truck->realtruckfilename);

	if (!newBeam)
	{
#ifdef USE_MYGUI
		RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, _L("unable to load new truck: limit reached. Please restart RoR"), "error.png");
#endif // USE_MYGUI
		return;
	}

	// remove the old truck
	curr_truck->state = RECYCLE;

	// copy over the most basic info
	if (curr_truck->free_node == newBeam->free_node)
	{
		for (int i=0;i<curr_truck->free_node;i++)
		{
			// copy over nodes attributes if the amount of them didnt change
			newBeam->nodes[i].AbsPosition = curr_truck->nodes[i].AbsPosition;
			newBeam->nodes[i].RelPosition = curr_truck->nodes[i].RelPosition;
			newBeam->nodes[i].Velocity    = curr_truck->nodes[i].Velocity;
			newBeam->nodes[i].Forces      = curr_truck->nodes[i].Forces;
			newBeam->nodes[i].iPosition   = curr_truck->nodes[i].iPosition;
			newBeam->nodes[i].smoothpos   = curr_truck->nodes[i].smoothpos;
		}
	}

	// TODO:
	// * copy over the engine infomation
	// * commands status
	// * other minor stati

	// notice the user about the amount of possible reloads
	String msg = TOSTRING(newBeam->trucknum) + String(" of ") + TOSTRING(MAX_TRUCKS) + String(" possible reloads.");
#ifdef USE_MYGUI
	RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, msg, "information.png");
#endif //USE_MYGUI

	// dislocate the old truck, so its out of sight
	curr_truck->resetPosition(100000, 100000, false, 100000);
	// note: in some point in the future we would delete the truck here,
	// but since this function is buggy we don't do it yet.

	// enter the new truck
	BeamFactory::getSingleton().setCurrentTruck(newBeam->trucknum);
}
