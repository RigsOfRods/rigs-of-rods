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
	along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

/** 
	@file   GUI_GameMSettings.h
	@author Moncef Ben Slimane
	@date   11/2014
*/

#include "ForwardDeclarations.h"
#include "GUI_GameSettingsLayout.h"

#include "GUI_GameSettings.h"
#include "InputEngine.h"

#include <OIS.h>

namespace RoR
{

namespace GUI
{

class GameSettings: public GameSettingsLayout
{

public:
	GameSettings();
	~GameSettings();

	void Show();
	void Hide(bool isMenu = true);
	
private:
	//Basic things
	void notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name);
	void LoadSettings();
	void UpdateControls();
	void SaveSettings();

	//Buttons
	void eventMouseButtonClickSaveButton(MyGUI::WidgetPtr _sender);
	void eventMouseButtonClickRegenCache(MyGUI::WidgetPtr _sender);
	void eventMouseButtonClickClearCache(MyGUI::WidgetPtr _sender);

	//Checkboxes
	void OnArcadeModeCheck(MyGUI::WidgetPtr _sender);
	void OnCameraPitchCheck(MyGUI::WidgetPtr _sender);
	void OnCreakSoundCheck(MyGUI::WidgetPtr _sender);
	void OnVSyncCheck(MyGUI::WidgetPtr _sender);
	void OnFullsreenCheck(MyGUI::WidgetPtr _sender);
	void OnShadowOptimCheck(MyGUI::WidgetPtr _sender);
	void OnWaveEnableCheck(MyGUI::WidgetPtr _sender);
	void OnParticleSystemCheck(MyGUI::WidgetPtr _sender);
	void OnHeatHazeCheck(MyGUI::WidgetPtr _sender);
	void OnMirrorsCheck(MyGUI::WidgetPtr _sender);
	void OnMotionBlurCheck(MyGUI::WidgetPtr _sender);
	void OnHQReflectionsCheck(MyGUI::WidgetPtr _sender);
	void OnHDRCheck(MyGUI::WidgetPtr _sender);
	void OnDOFCheck(MyGUI::WidgetPtr _sender);
	void OnSkidmarksCheck(MyGUI::WidgetPtr _sender);
	void OnGlowCheck(MyGUI::WidgetPtr _sender);
	void OnSunburnCheck(MyGUI::WidgetPtr _sender);
	void OnMultiThreadCheck(MyGUI::WidgetPtr _sender);
	void OnInterColisCheck(MyGUI::WidgetPtr _sender);
	void OnIntraColisCheck(MyGUI::WidgetPtr _sender);
	void OnASyncPhysicsCheck(MyGUI::WidgetPtr _sender);
	void OnDigitalSpeedoCheck(MyGUI::WidgetPtr _sender);
	void OnReplayEnableCheck(MyGUI::WidgetPtr _sender);
	void OnHqScreenshotsCheck(MyGUI::WidgetPtr _sender);
	void OnChatBoxAutoHideCheck(MyGUI::WidgetPtr _sender);

	//Sliders
	void OnVolumeSlider(MyGUI::ScrollBar* _sender, size_t _position);
	void OnFPSLimiterSlider(MyGUI::ScrollBar* _sender, size_t _position);
	void OnSightRangeSlider(MyGUI::ScrollBar* _sender, size_t _position);

	//Key mapping things
	void OnTabChange(MyGUI::TabControl* _sender, size_t _index);
	void LoadKeyMap();
	void OnKeymapTypeChange(MyGUI::ComboBox* _sender, size_t _index);
	bool isKeyMapLoaded;
	void OnReMapPress(MyGUI::WidgetPtr _sender);
	void FrameEntered(float dt);
	bool startCounter;
	unsigned long endTime;
	Ogre::String LastKeyCombo;
	bool isFrameActivated;

	std::map<int, std::vector<event_trigger_t> > KeyMap;

	//Few things
	std::map<std::string, std::string> GameSettingsMap;
	std::map<std::string, std::string> ExGameSettingsMap; //Store old settings to compare

	std::map<std::string, std::string> OgreSettingsMap;
	std::map<std::string, std::string> ExOgreSettingsMap; //Store old settings to compare

	bool IsLoaded;
	bool ShowRestartNotice;
};

} // namespace GUI

} // namespace RoR
