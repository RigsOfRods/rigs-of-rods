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
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   GUIManager.h
	@author based on the basemanager code from mygui common
*/

#include "GUIManager.h"

#include "Application.h"
#include "BeamFactory.h"
#include "ContentManager.h"
#include "Language.h"
#include "OgreSubsystem.h"
#include "RoRWindowEventUtilities.h"
#include "RTTLayer.h"
#include "Settings.h"
#include "TerrainManager.h"

//Managed GUI panels
#include "GUI_DebugOptions.h"
#include "GUI_FrictionSettings.h"
#include "GUI_GameMainMenu.h"
#include "GUI_GameAbout.h"
#include "GUI_GameConsole.h"
#include "GUI_GameSettings.h"
#include "GUI_GamePauseMenu.h"
#include "GUI_GameChatBox.h"
#include "GUI_LoadingWindow.h"
#include "GUI_MessageBox.h"
#include "GUI_MultiplayerSelector.h"
#include "GUI_MultiplayerClientList.h"
#include "GUI_MainSelector.h"
#include "GUI_RigSpawnerReportWindow.h"
#include "GUI_SimUtils.h"
#include "GUI_TextureToolWindow.h"
#include "GUI_TopMenubar.h"
#include "GUI_VehicleDescription.h"

#include <MyGUI.h>
#include <MyGUI_OgrePlatform.h>

#define RESOURCE_FILENAME "MyGUI_Core.xml"

namespace RoR {

struct GuiManagerImpl
{
    GuiManagerImpl():
        mygui(nullptr),
        mygui_platform(nullptr)
    {}

    GUI::GameMainMenu           panel_GameMainMenu;
    GUI::GameAbout              panel_GameAbout;
    GUI::GameSettings           panel_GameSettings;
    GUI::GamePauseMenu          panel_GamePauseMenu;
    GUI::DebugOptions           panel_DebugOptions;
    GUI::SimUtils               panel_SimUtils;
    GUI::gMessageBox            panel_MessageBox;
    GUI::MultiplayerSelector    panel_MultiplayerSelector;
    GUI::MainSelector           panel_MainSelector;
    GUI::GameChatBox            panel_ChatBox;
    GUI::RigSpawnerReportWindow panel_SpawnerReport;
    GUI::VehicleDescription     panel_VehicleDescription;
    GUI::MpClientList           panel_MpClientList;
    GUI::FrictionSettings       panel_FrictionSettings;
    GUI::TextureToolWindow      panel_TextureToolWindow;
    GUI::LoadingWindow          panel_LoadingWindow;
    GUI::TopMenubar             panel_TopMenubar;
    RoR::Console                panel_GameConsole;

    MyGUI::Gui*                 mygui;
    MyGUI::OgrePlatform*        mygui_platform;
};

void GUIManager::SetVisible_GameMainMenu        (bool v) { m_impl->panel_GameMainMenu       .SetVisible(v); }
void GUIManager::SetVisible_GameAbout           (bool v) { m_impl->panel_GameAbout          .SetVisible(v); }
void GUIManager::SetVisible_GameSettings        (bool v) { m_impl->panel_GameSettings       .SetVisible(v); }
void GUIManager::SetVisible_GamePauseMenu       (bool v) { m_impl->panel_GamePauseMenu      .SetVisible(v); }
void GUIManager::SetVisible_DebugOptions        (bool v) { m_impl->panel_DebugOptions       .SetVisible(v); }
void GUIManager::SetVisible_MultiplayerSelector (bool v) { m_impl->panel_MultiplayerSelector.SetVisible(v); }
void GUIManager::SetVisible_ChatBox             (bool v) { m_impl->panel_ChatBox            .SetVisible(v); }
void GUIManager::SetVisible_SpawnerReport       (bool v) { m_impl->panel_SpawnerReport      .SetVisible(v); }
void GUIManager::SetVisible_VehicleDescription  (bool v) { m_impl->panel_VehicleDescription .SetVisible(v); }
void GUIManager::SetVisible_MpClientList        (bool v) { m_impl->panel_MpClientList       .SetVisible(v); }
void GUIManager::SetVisible_FrictionSettings    (bool v) { m_impl->panel_FrictionSettings   .SetVisible(v); }
void GUIManager::SetVisible_TextureToolWindow   (bool v) { m_impl->panel_TextureToolWindow  .SetVisible(v); }
void GUIManager::SetVisible_LoadingWindow       (bool v) { m_impl->panel_LoadingWindow      .SetVisible(v); }
void GUIManager::SetVisible_TopMenubar          (bool v) { m_impl->panel_TopMenubar         .SetVisible(v); }
void GUIManager::SetVisible_Console             (bool v) { m_impl->panel_GameConsole        .SetVisible(v); }

bool GUIManager::IsVisible_GameMainMenu         () { return m_impl->panel_GameMainMenu       .IsVisible(); }
bool GUIManager::IsVisible_GameAbout            () { return m_impl->panel_GameAbout          .IsVisible(); }
bool GUIManager::IsVisible_GameSettings         () { return m_impl->panel_GameSettings       .IsVisible(); }
bool GUIManager::IsVisible_GamePauseMenu        () { return m_impl->panel_GamePauseMenu      .IsVisible(); }
bool GUIManager::IsVisible_DebugOptions         () { return m_impl->panel_DebugOptions       .IsVisible(); }
bool GUIManager::IsVisible_MessageBox           () { return m_impl->panel_MessageBox         .IsVisible(); }
bool GUIManager::IsVisible_MultiplayerSelector  () { return m_impl->panel_MultiplayerSelector.IsVisible(); }
bool GUIManager::IsVisible_MainSelector         () { return m_impl->panel_MainSelector       .IsVisible(); }
bool GUIManager::IsVisible_ChatBox              () { return m_impl->panel_ChatBox            .IsVisible(); }
bool GUIManager::IsVisible_SpawnerReport        () { return m_impl->panel_SpawnerReport      .IsVisible(); }
bool GUIManager::IsVisible_VehicleDescription   () { return m_impl->panel_VehicleDescription .IsVisible(); }
bool GUIManager::IsVisible_MpClientList         () { return m_impl->panel_MpClientList       .IsVisible(); }
bool GUIManager::IsVisible_FrictionSettings     () { return m_impl->panel_FrictionSettings   .IsVisible(); }
bool GUIManager::IsVisible_TextureToolWindow    () { return m_impl->panel_TextureToolWindow  .IsVisible(); }
bool GUIManager::IsVisible_LoadingWindow        () { return m_impl->panel_LoadingWindow      .IsVisible(); }
bool GUIManager::IsVisible_TopMenubar           () { return m_impl->panel_TopMenubar         .IsVisible(); }
bool GUIManager::IsVisible_Console              () { return m_impl->panel_GameConsole        .IsVisible(); }

// GUI GetInstance*()
Console*                    GUIManager::GetConsole()           { return &m_impl->panel_GameConsole         ; }
GUI::MainSelector*          GUIManager::GetMainSelector()      { return &m_impl->panel_MainSelector        ; }
GUI::LoadingWindow*         GUIManager::GetLoadingWindow()     { return &m_impl->panel_LoadingWindow       ; }
GUI::MpClientList*          GUIManager::GetMpClientList()      { return &m_impl->panel_MpClientList        ; }
GUI::MultiplayerSelector*   GUIManager::GetMpSelector()        { return &m_impl->panel_MultiplayerSelector ; }
GUI::FrictionSettings*      GUIManager::GetFrictionSettings()  { return &m_impl->panel_FrictionSettings    ; }
GUI::SimUtils*              GUIManager::GetSimUtils()          { return &m_impl->panel_SimUtils            ; }
GUI::TopMenubar*            GUIManager::GetTopMenubar()        { return &m_impl->panel_TopMenubar          ; }

GUIManager::GUIManager() :
    m_renderwindow_closed(false),
    m_impl(nullptr)
{
    RoR::App::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(this);
    RoRWindowEventUtilities::addWindowEventListener(RoR::App::GetOgreSubsystem()->GetRenderWindow(), this);

    std::string gui_logfilename = App::GetSysLogsDir() + PATH_SLASH + "MyGUI.log";
    auto mygui_platform = new MyGUI::OgrePlatform();
    mygui_platform->initialise(
        RoR::App::GetOgreSubsystem()->GetRenderWindow(), 
        gEnv->sceneManager,
        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
        gui_logfilename); // use cache resource group so preview images are working
    auto mygui = new MyGUI::Gui();

    // empty init
    mygui->initialise("");

    // add layer factory
    MyGUI::FactoryManager::getInstance().registerFactory<MyGUI::RTTLayer>("Layer");

    // then load the actual config
    MyGUI::ResourceManager::getInstance().load(RESOURCE_FILENAME);

    MyGUI::ResourceManager::getInstance().load(LanguageEngine::getSingleton().getMyGUIFontConfigFilename());

    m_impl = new GuiManagerImpl();
    m_impl->mygui_platform = mygui_platform;
    m_impl->mygui = mygui;

    // move the mouse into the middle of the screen, assuming we start at the top left corner (0,0)
    MyGUI::InputManager::getInstance().injectMouseMove(RoR::App::GetOgreSubsystem()->GetRenderWindow()->getWidth()*0.5f, RoR::App::GetOgreSubsystem()->GetRenderWindow()->getHeight()*0.5f, 0);
    MyGUI::PointerManager::getInstance().setVisible(true);
#ifdef _WIN32
    MyGUI::LanguageManager::getInstance().eventRequestTag = MyGUI::newDelegate(this, &GUIManager::eventRequestTag);
#endif // _WIN32
    windowResized(RoR::App::GetOgreSubsystem()->GetRenderWindow());
}

GUIManager::~GUIManager()
{
    delete m_impl;
}

void GUIManager::UnfocusGui()
{
    MyGUI::InputManager::getInstance().resetKeyFocusWidget();
    MyGUI::InputManager::getInstance().resetMouseCaptureWidget();
}

void GUIManager::ShutdownMyGUI()
{
    delete m_impl;

    if (m_impl->mygui)
    {
        m_impl->mygui->shutdown();
        delete m_impl->mygui;
        m_impl->mygui = nullptr;
    }

    if (m_impl->mygui_platform)
    {
        m_impl->mygui_platform->shutdown();
        delete m_impl->mygui_platform;
        m_impl->mygui_platform = nullptr;
    }
}

bool GUIManager::frameStarted(const Ogre::FrameEvent& evt)
{
	if (m_renderwindow_closed) return false;
	if (!m_impl->mygui) return true;


	// now hide the mouse cursor if not used since a long time
	if (getLastMouseMoveTime() > 5000)
	{
		MyGUI::PointerManager::getInstance().setVisible(false);
		//RoR::App::GetGuiManager()->GetTopMenubar()->setVisible(false);
	}

	return true;
}

bool GUIManager::frameEnded(const Ogre::FrameEvent& evt)
{
	return true;
};

void GUIManager::framestep(float dt)
{
    m_impl->panel_SimUtils.framestep(dt);
};

void GUIManager::PushNotification(Ogre::String Title, Ogre::UTFString text)
{
    m_impl->panel_SimUtils.PushNotification(Title, text);
}

void GUIManager::HideNotification()
{
    m_impl->panel_SimUtils.HideNotificationBox();
}

void GUIManager::windowResized(Ogre::RenderWindow* rw)
{
	int width = (int)rw->getWidth();
	int height = (int)rw->getHeight();
	setInputViewSize(width, height);

	BeamFactory *bf = BeamFactory::getSingletonPtr();
	if (bf) bf->windowResized();

	this->AdjustMainMenuPosition();
}

void GUIManager::windowClosed(Ogre::RenderWindow* rw)
{
	m_renderwindow_closed = true;
}

void GUIManager::eventRequestTag(const MyGUI::UString& _tag, MyGUI::UString& _result)
{
	_result = MyGUI::LanguageManager::getInstance().getTag(_tag);
}

Ogre::String GUIManager::getRandomWallpaperImage()
{
	using namespace Ogre;
	FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo("Wallpapers", "*.jpg", false);
	if (files.isNull() || files->empty())
	{
		files = ResourceGroupManager::getSingleton().findResourceFileInfo("Wallpapers", "*.png", false);
		if (files.isNull() || files->empty())
			return "";
	}
	srand ( time(NULL) );

	int num = 0;
	for (int i = 0; i<Math::RangeRandom(0, 10); i++)
		num = Math::RangeRandom(0, files->size());

	return files->at(num).filename;
}

void GUIManager::SetSceneManagerForGuiRendering(Ogre::SceneManager* scene_manager)
{
	m_impl->mygui_platform->getRenderManagerPtr()->setSceneManager(scene_manager);
}

void GUIManager::AdjustMainMenuPosition()
{
    Ogre::Viewport* viewport = RoR::App::GetOgreSubsystem()->GetRenderWindow()->getViewport(0);
    int margin = (viewport->getActualHeight() / 15);
    int top = viewport->getActualHeight() - m_impl->panel_GameMainMenu.GetHeight() - margin;
    m_impl->panel_GameMainMenu.SetPosition(margin, top);
}

void GUIManager::UpdateSimUtils(float dt, Beam *truck)
{
    if (m_impl->panel_SimUtils.IsBaseVisible()) //Better to update only when it's visible.
    {
        m_impl->panel_SimUtils.UpdateStats(dt, truck);
    }
}

void GUIManager::ShowMessageBox(Ogre::String mTitle, Ogre::String mText, bool button1, Ogre::String mButton1, bool AllowClose = false, bool button2 = false, Ogre::String mButton2 = "")
{
    m_impl->panel_MessageBox.ShowMessageBox(mTitle, mText, button1, mButton1, AllowClose, button2, mButton2);
}

void GUIManager::UpdateMessageBox(Ogre::String mTitle, Ogre::String mText, bool button1, Ogre::String mButton1, bool AllowClose = false, bool button2 = false, Ogre::String mButton2 = "", bool IsVisible = true)
{
    m_impl->panel_MessageBox.UpdateMessageBox(mTitle, mText, button1, mButton1, AllowClose, button2, mButton2, IsVisible);
}

int GUIManager::getMessageBoxResult()
{
    return m_impl->panel_MessageBox.getResult();
}

void GUIManager::InitMainSelector(RoR::SkinManager* skin_manager)
{
// todo remove
}

void GUIManager::AdjustPauseMenuPosition()
{
	Ogre::Viewport* viewport = RoR::App::GetOgreSubsystem()->GetRenderWindow()->getViewport(0);
	int margin = (viewport->getActualHeight() / 15);
	m_impl->panel_GamePauseMenu.SetPosition(
		margin, // left
		viewport->getActualHeight() - m_impl->panel_GamePauseMenu.GetHeight() - margin // top
		);
}

void GUIManager::AddRigLoadingReport(std::string const & vehicle_name, std::string const & text, int num_errors, int num_warnings, int num_other)
{
	m_impl->panel_SpawnerReport.SetRigLoadingReport(vehicle_name, text, num_errors, num_warnings, num_other);
}

void GUIManager::CenterSpawnerReportWindow()
{
	m_impl->panel_SpawnerReport.CenterToScreen();
}

void GUIManager::pushMessageChatBox(Ogre::String txt)
{
    m_impl->panel_ChatBox.pushMsg(txt);
}

void GUIManager::hideGUI(bool hidden)
{
    if (hidden)
    {
        m_impl->panel_SimUtils.HideNotificationBox();
        m_impl->panel_SimUtils.SetFPSBoxVisible(false);
        m_impl->panel_SimUtils.SetTruckInfoBoxVisible(false);
        m_impl->panel_ChatBox.Hide();
    }
    m_impl->panel_SimUtils.DisableNotifications(hidden);
}

void GUIManager::FrictionSettingsUpdateCollisions()
{
    App::GetGuiManager()->GetFrictionSettings()->setCollisions(gEnv->collisions);
}

void GUIManager::ReflectGameState()
{
    const auto app_state = App::GetActiveAppState();
    const auto mp_state  = App::GetActiveMpState();
    if (app_state == App::APP_STATE_MAIN_MENU)
    {
        m_impl->panel_GameMainMenu       .SetVisible(!m_impl->panel_MainSelector.IsVisible());

        m_impl->panel_TopMenubar         .SetVisible(false);
        m_impl->panel_ChatBox            .SetVisible(false);
        m_impl->panel_DebugOptions       .SetVisible(false);
        m_impl->panel_FrictionSettings   .SetVisible(false);
        m_impl->panel_GamePauseMenu      .SetVisible(false);
        m_impl->panel_TextureToolWindow  .SetVisible(false);
        m_impl->panel_VehicleDescription .SetVisible(false);
        m_impl->panel_SpawnerReport      .SetVisible(false);
        m_impl->panel_SimUtils           .SetBaseVisible(false);
        m_impl->panel_MpClientList       .SetVisible(mp_state == App::MP_STATE_CONNECTED);
        return;
    }
    if (app_state == App::APP_STATE_SIMULATION)
    {
        m_impl->panel_TopMenubar         .SetVisible(true);
        m_impl->panel_TopMenubar         .ReflectMultiplayerState();
        m_impl->panel_SimUtils           .SetBaseVisible(true);
        m_impl->panel_GameMainMenu       .SetVisible(false);
        return;
    }
}

} // namespace RoR
