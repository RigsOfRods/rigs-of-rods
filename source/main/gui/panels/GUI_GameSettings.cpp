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
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   
	@author Moncef Ben Slimane
	@date   11/2014
*/

#include "GUI_GameSettings.h"

#include "RoRPrerequisites.h"
#include "Utils.h"
#include "RoRVersion.h"
#include "rornet.h"
#include "Language.h"
#include "SelectorWindow.h"
#include "GlobalEnvironment.h"
#include "Application.h"
#include "RoRFrameListener.h"
#include "MainThread.h"
#include "OgreSubsystem.h"
#include "ImprovedConfigFile.h"
#include "GUIManager.h"
#include "Settings.h"

#include <MyGUI.h>

#ifdef USE_OPENAL
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#endif // USE_OPENAL

using namespace RoR;
using namespace GUI;

#define CLASS        GameSettings
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

CLASS::CLASS()
{
	IsLoaded = false;
	ShowRestartNotice = false;

	MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);
	win->eventWindowButtonPressed += MyGUI::newDelegate(this, &CLASS::notifyWindowButtonPressed); //The "X" button thing

	//Buttons
	m_savebtn->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickSaveButton);
	m_regen_cache->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickRegenCache);
	m_clear_cache->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickClearCache);

	//Checkboxes
	m_arc_mode->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnArcadeModeCheck);
	m_d_cam_pitch->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnCameraPitchCheck);
	m_d_creak_sound->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnCreakSoundCheck);
	m_vsync->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnVSyncCheck);
	m_fullscreen->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnFullsreenCheck);
	m_sh_pf_opti->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnShadowOptimCheck);
	m_e_waves->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnWaveEnableCheck);

	m_psystem->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnParticleSystemCheck);
	m_heathaze->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnHeatHazeCheck);
	m_mirrors->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnMirrorsCheck);
	m_mblur->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnMotionBlurCheck);
	m_hq_ref->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnHQReflectionsCheck);
	m_hdr->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnHDRCheck);
	m_dof->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnDOFCheck);
	m_skidmarks->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnSkidmarksCheck);
	m_glow->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnGlowCheck);
	m_sunburn->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnSunburnCheck);

	m_disable_multithreading->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnMultiThreadCheck);
	m_disable_inter_collsion->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnInterColisCheck);
	m_disable_intra_collision->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnIntraColisCheck);
	m_enable_async_physics->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnASyncPhysicsCheck);

	m_digital_speedo->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnDigitalSpeedoCheck);

	m_enable_replay->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnReplayEnableCheck);

	//Sliders
	m_volume_slider->eventScrollChangePosition += MyGUI::newDelegate(this, &CLASS::OnVolumeSlider);
	m_fps_limiter_slider->eventScrollChangePosition += MyGUI::newDelegate(this, &CLASS::OnFPSLimiterSlider);
	m_sightrange->eventScrollChangePosition += MyGUI::newDelegate(this, &CLASS::OnSightRangeSlider);

	MyGUI::IntSize gui_area = MyGUI::RenderManager::getInstance().getViewSize();
	mMainWidget->setPosition(gui_area.width/2 - mMainWidget->getWidth()/2, gui_area.height/2 - mMainWidget->getHeight()/2);

	Hide();
}

CLASS::~CLASS()
{

}

void CLASS::Show()
{
	MAIN_WIDGET->setVisibleSmooth(true);
	LoadSettings();
	IsLoaded = true; //Avoid things being loaded more than once
	ShowRestartNotice = false; //Reinit
}

void CLASS::Hide(bool isMenu)
{
	MAIN_WIDGET->setVisibleSmooth(false);
	RoR::Application::GetGuiManager()->ShowMainMenu(isMenu);
}

void CLASS::notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name)
{
	if (_name == "close")
		Hide();
}

void CLASS::eventMouseButtonClickSaveButton(MyGUI::WidgetPtr _sender)
{
	SaveSettings();

	if (ShowRestartNotice == true)
	{
		RoR::Application::GetGuiManager()->ShowMessageBox("Restart required", "You need to restart the game for few settings to apply.", true, "Ok", true, false, "");
	}

	Hide();
}

void CLASS::LoadSettings()
{
	//Load the settings from the game it self, better and faster.
	GameSettingsMap = Settings::getSingleton().GetSettingMap();
	ExGameSettingsMap = Settings::getSingleton().GetSettingMap();

	if (GameSettingsMap["Allow NVPerfHUD"] == "") GameSettingsMap["Allow NVPerfHUD"] = "No";
	if (GameSettingsMap["Floating-point mode"] == "") GameSettingsMap["Floating-point mode"] = "Fastest";
	if (GameSettingsMap["Colour Depth"] == "") GameSettingsMap["Colour Depth"] = "32";
	UpdateControls();
}

void CLASS::UpdateControls()
{

	int valuecounter = 0; // Going to be usefull for selections

	//Lang (Still not done)
	if (!IsLoaded)
	{
		m_lang->addItem("English (U.S.)");
	}
	m_lang->setIndexSelected(0); //TODO

	if (!IsLoaded)
	{
		m_gearbox_mode->addItem("Automatic shift");
		m_gearbox_mode->addItem("Manual shift - Auto clutch");
		m_gearbox_mode->addItem("Fully Manual: sequential shift");
		m_gearbox_mode->addItem("Fully Manual: stick shift");
		m_gearbox_mode->addItem("Fully Manual: stick shift with ranges");
	}

	//Gearbox
	Ogre::String gearbox_mode = GameSettingsMap["GearboxMode"];
	if (gearbox_mode == "Manual shift - Auto clutch")
		m_gearbox_mode->setIndexSelected(1);
	else if (gearbox_mode == "Fully Manual: sequential shift")
		m_gearbox_mode->setIndexSelected(2);
	else if (gearbox_mode == "Fully Manual: stick shift")
		m_gearbox_mode->setIndexSelected(3);
	else if (gearbox_mode == "Fully Manual: stick shift with ranges")
		m_gearbox_mode->setIndexSelected(4);
	else
		m_gearbox_mode->setIndexSelected(0);


	Ogre::RenderSystem* rs = Ogre::Root::getSingleton().getRenderSystem();
	// add all rendersystems to the list
	if (m_render_sys->getItemCount() == 0)
	{
		const Ogre::RenderSystemList list = Application::GetOgreSubsystem()->GetOgreRoot()->getAvailableRenderers();
		int selection = 0;
		for (Ogre::RenderSystemList::const_iterator it = list.begin(); it != list.end(); it++, valuecounter++)
		{
			if (rs && rs->getName() == (*it)->getName())
			{
				ExOgreSettingsMap["Render System"] = rs->getName();
				selection = valuecounter;
			}
			else if (!rs) {
				LOG("Error: No Ogre Render System found");
			}
			if (!IsLoaded)
			{
				m_render_sys->addItem(Ogre::String((*it)->getName()));
			}
		}
		m_render_sys->setIndexSelected(selection);
	}

	Ogre::ConfigFile cfg;
	cfg.load(SSETTING("ogre.cfg", "ogre.cfg"));

	//Few GameSettingsMap
	Ogre::String bFullScreen = cfg.getSetting("Full Screen", rs->getName());
	if (bFullScreen == "Yes")
	{
		ExOgreSettingsMap["Full Screen"] = "Yes";
		m_fullscreen->setStateCheck(true);
	}
	else
	{
		ExOgreSettingsMap["Full Screen"] = "No";
		m_fullscreen->setStateCheck(false);
	}

	Ogre::String bVsync = cfg.getSetting("VSync", rs->getName());
	if (bVsync == "Yes")
	{
		ExOgreSettingsMap["VSync"] = "Yes";
		m_vsync->setStateCheck(true);
	}
	else
	{
		ExOgreSettingsMap["VSync"] = "No";
		m_vsync->setStateCheck(false);
	}
		
	// store available rendering devices and available resolutions
	Ogre::ConfigOptionMap& CurrentRendererOptions = rs->getConfigOptions();
	Ogre::ConfigOptionMap::iterator configItr = CurrentRendererOptions.begin();
	Ogre::StringVector mFoundResolutions;
	Ogre::StringVector mFoundFSAA;
	while (configItr != CurrentRendererOptions.end())
	{
		if ((configItr)->first == "Video Mode")
		{
			// Store Available Resolutions
			mFoundResolutions = ((configItr)->second.possibleValues);
		}
		if ((configItr)->first == "FSAA")
		{
			// Store Available Resolutions
			mFoundFSAA = ((configItr)->second.possibleValues);
		}
		configItr++;
	}

	//Loop thru the vector for the resolutions
	valuecounter = 0; //Back to default
	Ogre::StringVector::iterator iterRes = mFoundResolutions.begin();
	for (; iterRes != mFoundResolutions.end(); iterRes++)
	{
		if (!IsLoaded)
		{
			m_resolution->addItem(Ogre::String((iterRes)->c_str()));
		}
		if ((iterRes)->c_str() == cfg.getSetting("Video Mode", rs->getName()))
		{
			ExOgreSettingsMap["Video Mode"] = (iterRes)->c_str();
			m_resolution->setIndexSelected(valuecounter);
		}
		valuecounter++;
	}

	//Loop thru the vector for the FSAAs
	valuecounter = 0; //Back to default
	Ogre::StringVector::iterator iterFSAA = mFoundFSAA.begin();
	for (; iterFSAA != mFoundFSAA.end(); iterFSAA++)
	{
		if (!IsLoaded)
		{
			m_fsaa->addItem(Ogre::String((iterFSAA)->c_str()));
		}
		if ((iterFSAA)->c_str() == cfg.getSetting("FSAA", rs->getName()))
		{
			ExOgreSettingsMap["FSAA"] = (iterFSAA)->c_str();
			m_fsaa->setIndexSelected(valuecounter);
		}
		
		valuecounter++;
	}

	//Few GameSettingsMap
	if (GameSettingsMap["ArcadeControls"] == "Yes")
		m_arc_mode->setStateCheck(true);
	else
		m_arc_mode->setStateCheck(false);

	if (GameSettingsMap["External Camera Mode"] == "Static")
		m_d_cam_pitch->setStateCheck(true);
	else
		m_d_cam_pitch->setStateCheck(false);

	if (GameSettingsMap["Creak Sound"] == "No")
		m_d_creak_sound->setStateCheck(true);
	else
		m_d_creak_sound->setStateCheck(false);

	//Fov
	m_fovexternal->setCaption(GameSettingsMap["FOV External"]);
	m_fovinternal->setCaption(GameSettingsMap["FOV Internal"]);

	
	//Texture Filtering
	Ogre::String texfilter = GameSettingsMap["Texture Filtering"];
	if (texfilter == "Bilinear")
		m_tex_filter->setIndexSelected(1);
	else if (texfilter == "Trilinear")
		m_tex_filter->setIndexSelected(2);
	else if (texfilter == "Anisotropic (best looking)")
		m_tex_filter->setIndexSelected(3);
	else
		m_tex_filter->setIndexSelected(0);

	//Sky effects
	Ogre::String skytype = GameSettingsMap["Sky effects"];
	if (skytype == "Caelum (best looking, slower)")
		m_sky_type->setIndexSelected(1);
	else if (skytype == "SkyX (best looking, slower)")
		m_sky_type->setIndexSelected(2);
	else
		m_sky_type->setIndexSelected(0);

	//Shadow technique
	Ogre::String shadowtype = GameSettingsMap["Shadow technique"];
	if (shadowtype == "Texture shadows")
		m_shadow_type->setIndexSelected(1);
	else if (shadowtype == "Stencil shadows (best looking)")
		m_shadow_type->setIndexSelected(2);
	else if (shadowtype == "Parallel-split Shadow Maps")
		m_shadow_type->setIndexSelected(3);
	else
		m_shadow_type->setIndexSelected(0);

	//Water effects
	Ogre::String watertype = GameSettingsMap["Water effects"];
	if (watertype == "Reflection")
		m_water_type->setIndexSelected(1);
	else if (watertype == "Reflection + refraction (speed optimized)")
		m_water_type->setIndexSelected(2);
	else if (watertype == "Reflection + refraction (quality optimized)")
		m_water_type->setIndexSelected(3);
	else if (watertype == "Hydrax")
		m_water_type->setIndexSelected(4);
	else
		m_water_type->setIndexSelected(0);
		
	//Vegetation
	Ogre::String vegetationtype = GameSettingsMap["Vegetation"];
	if (vegetationtype == "20%")
		m_vegetation->setIndexSelected(1);
	else if (vegetationtype == "50%")
		m_vegetation->setIndexSelected(2);
	else if (vegetationtype == "Full (best looking, slower)")
		m_vegetation->setIndexSelected(3);
	else
		m_vegetation->setIndexSelected(0);

	//Light source effects
	Ogre::String lightstype = GameSettingsMap["Lights"];
	if (lightstype == "Only current vehicle, main lights")
		m_light_source_effects->setIndexSelected(1);
	else if (lightstype == "All vehicles, main lights")
		m_light_source_effects->setIndexSelected(2);
	else if (lightstype == "All vehicles, all lights")
		m_light_source_effects->setIndexSelected(3);
	else
		m_light_source_effects->setIndexSelected(0);
	
	//Speed until selection
	Ogre::String speedunit = GameSettingsMap["SpeedUnit"];
	if (speedunit == "Metric")
		m_speed_unit->setIndexSelected(1);
	else
		m_speed_unit->setIndexSelected(0);

	//Other configs
	if (GameSettingsMap["DigitalSpeedo"] == "Yes")
		m_digital_speedo->setStateCheck(true);
	else
		m_digital_speedo->setStateCheck(false);

	if (GameSettingsMap["Particles"] == "Yes")
		m_psystem->setStateCheck(true);
	else
		m_psystem->setStateCheck(false);

	if (GameSettingsMap["HeatHaze"] == "Yes")
		m_heathaze->setStateCheck(true);
	else
		m_heathaze->setStateCheck(false);

	if (GameSettingsMap["Mirrors"] == "Yes")
		m_mirrors->setStateCheck(true);
	else
		m_mirrors->setStateCheck(false);

	if (GameSettingsMap["Sunburn"] == "Yes")
		m_sunburn->setStateCheck(true);
	else
		m_sunburn->setStateCheck(false);

	if (GameSettingsMap["HDR"] == "Yes")
		m_hdr->setStateCheck(true);
	else
		m_hdr->setStateCheck(false);

	if (GameSettingsMap["Motion blur"] == "Yes")
		m_mblur->setStateCheck(true);
	else
		m_mblur->setStateCheck(false);

	if (GameSettingsMap["Skidmarks"] == "Yes")
		m_skidmarks->setStateCheck(true);
	else
		m_skidmarks->setStateCheck(false);

	if (GameSettingsMap["Envmap"] == "Yes")
		m_hq_ref->setStateCheck(true);
	else
		m_hq_ref->setStateCheck(false);

	if (GameSettingsMap["Glow"] == "Yes")
		m_glow->setStateCheck(true);
	else
		m_glow->setStateCheck(false);

	if (GameSettingsMap["DOF"] == "Yes")
		m_dof->setStateCheck(true);
	else
		m_dof->setStateCheck(false);

	if (GameSettingsMap["Waves"] == "Yes")
		m_e_waves->setStateCheck(true);
	else
		m_e_waves->setStateCheck(false);

	if (GameSettingsMap["Shadow optimizations"] == "Yes")
		m_sh_pf_opti->setStateCheck(true);
	else
		m_sh_pf_opti->setStateCheck(false);

	if (GameSettingsMap["AsynchronousPhysics"] == "Yes")
		m_enable_async_physics->setStateCheck(true);
	else
		m_enable_async_physics->setStateCheck(false);

	if (GameSettingsMap["DisableCollisions"] == "Yes")
		m_disable_inter_collsion->setStateCheck(true);
	else
		m_disable_inter_collsion->setStateCheck(false);

	if (GameSettingsMap["DisableSelfCollisions"] == "Yes")
		m_disable_intra_collision->setStateCheck(true);
	else
		m_disable_intra_collision->setStateCheck(false);

	if (GameSettingsMap["Multi-threading"] == "No")
		m_disable_multithreading->setStateCheck(true);
	else
		m_disable_multithreading->setStateCheck(false);

	//Volume slider
	long sound_volume = Ogre::StringConverter::parseLong(GameSettingsMap["Sound Volume"], 100);
	m_volume_slider->setScrollRange(101);
	m_volume_slider->setScrollPosition(sound_volume -1);
	if (m_volume_slider->getScrollPosition() >= 100)
		m_volume_indicator->setCaption("100%");
	else
		m_volume_indicator->setCaption(Ogre::StringConverter::toString(sound_volume) + "%");

	//Audio Devices
	valuecounter = 0; //Back to default
	char *devices = (char *)alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
	while (devices && *devices != 0)
	{
		if (!IsLoaded)
		{
			m_audio_dev->addItem(Ogre::String(devices));
		}
		if (Ogre::String(devices) == GameSettingsMap["AudioDevice"])
			m_audio_dev->setIndexSelected(valuecounter);

		devices += strlen(devices) + 1; //next device
		valuecounter++;
	}

	//FPS Limiter slider
	long fps_limit = Ogre::StringConverter::parseLong(GameSettingsMap["FPS-Limiter"], 60);
	m_fps_limiter_slider->setScrollRange(200);
	m_fps_limiter_slider->setScrollPosition(fps_limit -1);


	if (fps_limit >= 199)
		m_fps_limiter_indicator->setCaption("Unlimited");
	else
		m_fps_limiter_indicator->setCaption(Ogre::StringConverter::toString(fps_limit) + " FPS");

	//SightRange slider
	long sight_range = Ogre::StringConverter::parseLong(GameSettingsMap["SightRange"], 5000);
	m_sightrange->setScrollRange(5000);
	m_sightrange->setScrollPosition(sight_range -1);
	if (sight_range >= 4999)
		m_sightrange_indicator->setCaption("Unlimited");
	else
		m_sightrange_indicator->setCaption(Ogre::StringConverter::toString(sight_range) + " m");
			
}

void CLASS::OnArcadeModeCheck(MyGUI::WidgetPtr _sender)
{
	m_arc_mode->setStateCheck(!m_arc_mode->getStateCheck());
	if (m_arc_mode->getStateCheck() == false)
		GameSettingsMap["ArcadeControls"] = "No";
	else
		GameSettingsMap["ArcadeControls"] = "Yes";
}

void CLASS::OnCameraPitchCheck(MyGUI::WidgetPtr _sender)
{
	m_d_cam_pitch->setStateCheck(!m_d_cam_pitch->getStateCheck());
	if (m_d_cam_pitch->getStateCheck() == false)
		GameSettingsMap["External Camera Mode"] = "Pitching";
	else
		GameSettingsMap["External Camera Mode"] = "Static";
}

void CLASS::OnCreakSoundCheck(MyGUI::WidgetPtr _sender)
{
	m_d_creak_sound->setStateCheck(!m_d_creak_sound->getStateCheck());
	if (m_d_creak_sound->getStateCheck() == false)
		GameSettingsMap["Creak Sound"] = "No";
	else
		GameSettingsMap["Creak Sound"] = "Yes";
	ShowRestartNotice = true;
}

void CLASS::OnVSyncCheck(MyGUI::WidgetPtr _sender)
{
	m_vsync->setStateCheck(!m_vsync->getStateCheck());
	if (m_vsync->getStateCheck() == false)
		OgreSettingsMap["VSync"] = "No"; //This one is special
	else
		OgreSettingsMap["VSync"] = "Yes";
	ShowRestartNotice = true;
}

void CLASS::OnFullsreenCheck(MyGUI::WidgetPtr _sender)
{
	m_fullscreen->setStateCheck(!m_fullscreen->getStateCheck());
	if (m_fullscreen->getStateCheck() == false)
		OgreSettingsMap["Fullscreen"] = "No"; //This one is special
	else
		OgreSettingsMap["Fullscreen"] = "Yes";
	ShowRestartNotice = true;
}

void CLASS::OnShadowOptimCheck(MyGUI::WidgetPtr _sender)
{
	m_sh_pf_opti->setStateCheck(!m_sh_pf_opti->getStateCheck());
	if (m_sh_pf_opti->getStateCheck() == false)
		GameSettingsMap["Shadow optimizations"] = "No";
	else
		GameSettingsMap["Shadow optimizations"] = "Yes";
}

void CLASS::OnWaveEnableCheck(MyGUI::WidgetPtr _sender)
{
	m_e_waves->setStateCheck(!m_e_waves->getStateCheck());
	if (m_e_waves->getStateCheck() == false)
		GameSettingsMap["Waves"] = "No";
	else
		GameSettingsMap["Waves"] = "Yes";
}

void CLASS::OnParticleSystemCheck(MyGUI::WidgetPtr _sender)
{
	m_psystem->setStateCheck(!m_psystem->getStateCheck());
	if (m_psystem->getStateCheck() == false)
		GameSettingsMap["Particles"] = "No";
	else
		GameSettingsMap["Particles"] = "Yes";
}

void CLASS::OnHeatHazeCheck(MyGUI::WidgetPtr _sender)
{
	m_heathaze->setStateCheck(!m_heathaze->getStateCheck());
	if (m_heathaze->getStateCheck() == false)
		GameSettingsMap["HeatHaze"] = "No";
	else
		GameSettingsMap["HeatHaze"] = "Yes";
}

void CLASS::OnMirrorsCheck(MyGUI::WidgetPtr _sender)
{
	m_mirrors->setStateCheck(!m_mirrors->getStateCheck());
	if (m_mirrors->getStateCheck() == false)
		GameSettingsMap["Mirrors"] = "No";
	else
		GameSettingsMap["Mirrors"] = "Yes";
}

void CLASS::OnMotionBlurCheck(MyGUI::WidgetPtr _sender)
{
	m_mblur->setStateCheck(!m_mblur->getStateCheck());
	if (m_mblur->getStateCheck() == false)
		GameSettingsMap["Motion blur"] = "No";
	else
		GameSettingsMap["Motion blur"] = "Yes";
}

void CLASS::OnHQReflectionsCheck(MyGUI::WidgetPtr _sender)
{
	m_hq_ref->setStateCheck(!m_hq_ref->getStateCheck());
	if (m_hq_ref->getStateCheck() == false)
		GameSettingsMap["Envmap"] = "No";
	else
		GameSettingsMap["Envmap"] = "Yes";
}

void CLASS::OnHDRCheck(MyGUI::WidgetPtr _sender)
{
	m_hdr->setStateCheck(!m_hdr->getStateCheck());
	if (m_hdr->getStateCheck() == false)
		GameSettingsMap["HDR"] = "No";
	else
		GameSettingsMap["HDR"] = "Yes";
}

void CLASS::OnDOFCheck(MyGUI::WidgetPtr _sender)
{
	m_dof->setStateCheck(!m_dof->getStateCheck());
	if (m_dof->getStateCheck() == false)
		GameSettingsMap["DOF"] = "No";
	else
		GameSettingsMap["DOF"] = "Yes";
}

void CLASS::OnSkidmarksCheck(MyGUI::WidgetPtr _sender)
{
	m_skidmarks->setStateCheck(!m_skidmarks->getStateCheck());
	if (m_skidmarks->getStateCheck() == false)
		GameSettingsMap["Skidmarks"] = "No";
	else
		GameSettingsMap["Skidmarks"] = "Yes";
}

void CLASS::OnGlowCheck(MyGUI::WidgetPtr _sender)
{
	m_glow->setStateCheck(!m_glow->getStateCheck());
	if (m_glow->getStateCheck() == false)
		GameSettingsMap["Glow"] = "No";
	else
		GameSettingsMap["Glow"] = "Yes";
}

void CLASS::OnSunburnCheck(MyGUI::WidgetPtr _sender)
{
	m_sunburn->setStateCheck(!m_sunburn->getStateCheck());
	if (m_sunburn->getStateCheck() == false)
		GameSettingsMap["Sunburn"] = "No";
	else
		GameSettingsMap["Sunburn"] = "Yes";
}

void CLASS::OnMultiThreadCheck(MyGUI::WidgetPtr _sender)
{
	m_disable_multithreading->setStateCheck(!m_disable_multithreading->getStateCheck());
	if (m_disable_multithreading->getStateCheck() == false)
		GameSettingsMap["Multi-threading"] = "Yes";
	else
		GameSettingsMap["Multi-threading"] = "No";
	ShowRestartNotice = true;
}

void CLASS::OnInterColisCheck(MyGUI::WidgetPtr _sender)
{
	m_disable_inter_collsion->setStateCheck(!m_disable_inter_collsion->getStateCheck());
	if (m_disable_inter_collsion->getStateCheck() == false)
		GameSettingsMap["DisableCollisions"] = "No";
	else
		GameSettingsMap["DisableCollisions"] = "Yes";
	ShowRestartNotice = true;
}

void CLASS::OnIntraColisCheck(MyGUI::WidgetPtr _sender)
{
	m_disable_intra_collision->setStateCheck(!m_disable_intra_collision->getStateCheck());
	if (m_disable_intra_collision->getStateCheck() == false)
		GameSettingsMap["DisableSelfCollisions"] = "No";
	else
		GameSettingsMap["DisableSelfCollisions"] = "Yes";
	ShowRestartNotice = true;
}

void CLASS::OnASyncPhysicsCheck(MyGUI::WidgetPtr _sender)
{
	m_enable_async_physics->setStateCheck(!m_enable_async_physics->getStateCheck());
	if (m_enable_async_physics->getStateCheck() == false)
		GameSettingsMap["AsynchronousPhysics"] = "No";
	else
		GameSettingsMap["AsynchronousPhysics"] = "Yes";
	ShowRestartNotice = true;
}

void CLASS::OnDigitalSpeedoCheck(MyGUI::WidgetPtr _sender)
{
	m_digital_speedo->setStateCheck(!m_digital_speedo->getStateCheck());
	if (m_digital_speedo->getStateCheck() == false)
		GameSettingsMap["DigitalSpeedo"] = "No";
	else
		GameSettingsMap["DigitalSpeedo"] = "Yes";
	//ShowRestartNotice = true;
}

void CLASS::OnReplayEnableCheck(MyGUI::WidgetPtr _sender)
{
	m_enable_replay->setStateCheck(!m_enable_replay->getStateCheck());
	if (m_enable_replay->getStateCheck() == false)
		GameSettingsMap["Replay mode"] = "No";
	else
		GameSettingsMap["Replay mode"] = "Yes";
	//ShowRestartNotice = true;
}

void CLASS::OnVolumeSlider(MyGUI::ScrollBar* _sender, size_t _position)
{
	GameSettingsMap["Sound Volume"] = Ogre::StringConverter::toString(_position); //Erm, it's a string in the config map, isn't it?
	if (m_volume_slider->getScrollPosition() >= 100)
		m_volume_indicator->setCaption("100%");
	else
		m_volume_indicator->setCaption(Ogre::StringConverter::toString(_position) + "%");
}

void CLASS::OnFPSLimiterSlider(MyGUI::ScrollBar* _sender, size_t _position)
{
	GameSettingsMap["FPS-Limiter"] = Ogre::StringConverter::toString(_position); //Erm, it's a string in the config map, isn't it?
	if (_position >= 199)
		m_fps_limiter_indicator->setCaption("Unlimited");
	else
		m_fps_limiter_indicator->setCaption(Ogre::StringConverter::toString(_position) + " FPS");
}

void CLASS::OnSightRangeSlider(MyGUI::ScrollBar* _sender, size_t _position)
{
	GameSettingsMap["SightRange"] = Ogre::StringConverter::toString(_position); //Erm, it's a string in the config map, isn't it?
	if (_position >= 4999)
		m_sightrange_indicator->setCaption("Unlimited");
	else
		m_sightrange_indicator->setCaption(Ogre::StringConverter::toString(_position) + " m");
}

void CLASS::SaveSettings()
{
	//Get combo boxes values first
	//Check boxes and sliders values are updated as they are being used
	GameSettingsMap["GearboxMode"] = m_gearbox_mode->getCaption();
	OgreSettingsMap["Render System"] = m_render_sys->getCaption(); //This one is special 
	OgreSettingsMap["Video Mode"] = m_resolution->getCaption(); //This one is special 
	OgreSettingsMap["FSAA"] = m_fsaa->getCaption(); //This one is special 
	GameSettingsMap["FOV External"] = m_fovexternal->getCaption();
	GameSettingsMap["FOV Internal"] = m_fovinternal->getCaption();
	GameSettingsMap["Texture Filtering"] = m_tex_filter->getCaption();
	GameSettingsMap["Sky effects"] = m_sky_type->getCaption();
	GameSettingsMap["Shadow technique"] = m_shadow_type->getCaption();
	GameSettingsMap["Water effects"] = m_water_type->getCaption();
	GameSettingsMap["Vegetation"] = m_vegetation->getCaption();
	GameSettingsMap["Lights"] = m_light_source_effects->getCaption(); 
	GameSettingsMap["AudioDevice"] = m_audio_dev->getCaption();
	GameSettingsMap["SpeedUnit"] = m_speed_unit->getCaption();
	//That's it i think

	//Not sure if these settings really need a restart, temporary.
	if (GameSettingsMap["AudioDevice"] != ExGameSettingsMap["AudioDevice"])
		ShowRestartNotice = true;
	else if (GameSettingsMap["FOV External"] != ExGameSettingsMap["FOV External"] || GameSettingsMap["FOV Internal"] != ExGameSettingsMap["FOV Internal"])
		ShowRestartNotice = true;
	else if (OgreSettingsMap["Render System"] != ExOgreSettingsMap["Render System"])
		ShowRestartNotice = true;
	else if (OgreSettingsMap["Video Mode"] != ExOgreSettingsMap["Video Mode"])
		ShowRestartNotice = true;
	else if (OgreSettingsMap["FSAA"] != ExOgreSettingsMap["FSAA"])
		ShowRestartNotice = true;


	//Something used by both saves
	std::map<std::string, std::string>::iterator it;

	//Save ogre's settings
	Ogre::RenderSystem* rs = Ogre::Root::getSingleton().getRenderSystem();
	for (it = OgreSettingsMap.begin(); it != OgreSettingsMap.end(); it++)
	{
		try
		{
			rs->setConfigOption(it->first.c_str(), it->second.c_str());
		}
		catch (...)
		{
			LOG("Error setting Ogre Values");
		}
	}
	Ogre::String err = rs->validateConfigOptions();
	if (err.length() > 0)
	{
		LOG("Ogre config validation error");
	}
	else
	{
		Ogre::Root::getSingleton().saveConfig();
	}

	// now save the GameSettingsMap
	for (it = GameSettingsMap.begin(); it != GameSettingsMap.end(); it++)
	{
		if (it->first.c_str() == "User Token" || it->first.c_str() == "User Token Hash" || it->first.c_str() == "Config Root" || it->first.c_str() == "Cache Path" || it->first.c_str() == "Log Path" || it->first.c_str() == "Resources Path" || it->first.c_str() == "Program Path")
			return;

		Settings::getSingleton().setSetting(it->first.c_str(), it->second.c_str()); //Avoid restarting the game in few cases.
		Settings::getSingleton().saveSettings();
	}

} 

void CLASS::eventMouseButtonClickRegenCache(MyGUI::WidgetPtr _sender)
{
	MAIN_WIDGET->setVisibleSmooth(false);
	Application::GetMainThreadLogic()->RegenCache();
	MAIN_WIDGET->setVisibleSmooth(true);
}


void CLASS::eventMouseButtonClickClearCache(MyGUI::WidgetPtr _sender)
{
#ifdef _WIN32
//FindFirstFile/DeleteFile and all that didn't work. (I don't know why)

#endif
	//TODO: Linux/Mac
	//Hiradur should take a look at this.

	ShowRestartNotice = true;
}