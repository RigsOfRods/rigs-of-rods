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
        mygui_platform(nullptr),
        is_sim_utils_visible(false),
        is_mp_client_list_created(false)
    {}

    std::unique_ptr<GUI::GameMainMenu>           panel_GameMainMenu;
    std::unique_ptr<GUI::GameAbout>              panel_GameAbout;
    std::unique_ptr<GUI::GameSettings>           panel_GameSettings;
    std::unique_ptr<GUI::GamePauseMenu>          panel_GamePauseMenu;
    std::unique_ptr<GUI::DebugOptions>           panel_DebugOptions;
    std::unique_ptr<GUI::SimUtils>               panel_SimUtils;
    std::unique_ptr<GUI::gMessageBox>            panel_MessageBox;
    std::unique_ptr<GUI::MultiplayerSelector>    panel_MultiplayerSelector;
    std::shared_ptr<GUI::MainSelector>           panel_MainSelector;
    std::shared_ptr<GUI::GameChatBox>            panel_ChatBox;
    std::unique_ptr<GUI::RigSpawnerReportWindow> panel_SpawnerReport;
    std::unique_ptr<GUI::VehicleDescription>     panel_VehicleDescription;
    std::unique_ptr<GUI::MpClientList>           panel_MpClientList;
    std::unique_ptr<GUI::FrictionSettings>       panel_FrictionSettings;
    std::unique_ptr<GUI::TextureToolWindow>      panel_TextureToolWindow;
    std::unique_ptr<GUI::LoadingWindow>          panel_LoadingWindow;
    std::unique_ptr<GUI::TopMenubar>             panel_TopMenubar;
    std::unique_ptr<RoR::Console>                panel_GameConsole;

    MyGUI::Gui*                                  mygui;
    MyGUI::OgrePlatform*                         mygui_platform;
    bool                                         is_sim_utils_visible;
    bool                                         is_mp_client_list_created;
};

GUIManager::GUIManager() :
    m_renderwindow_closed(false),
    m_impl(nullptr)
{
	m_impl = new GuiManagerImpl();

	RoR::Application::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(this);
	RoRWindowEventUtilities::addWindowEventListener(RoR::Application::GetOgreSubsystem()->GetRenderWindow(), this);

	windowResized(RoR::Application::GetOgreSubsystem()->GetRenderWindow());
	createGui();
#ifdef _WIN32
	MyGUI::LanguageManager::getInstance().eventRequestTag = MyGUI::newDelegate(this, &GUIManager::eventRequestTag);
#endif // _WIN32

    // Create panels
    m_impl->panel_SpawnerReport       = std::unique_ptr<GUI::RigSpawnerReportWindow>(new GUI::RigSpawnerReportWindow());
    m_impl->panel_VehicleDescription  = std::unique_ptr<GUI::VehicleDescription>    (new GUI::VehicleDescription    ());
    m_impl->panel_ChatBox             = std::unique_ptr<GUI::GameChatBox>           (new GUI::GameChatBox           ());
    m_impl->panel_DebugOptions        = std::unique_ptr<GUI::DebugOptions>          (new GUI::DebugOptions          ());
    m_impl->panel_GameMainMenu        = std::unique_ptr<GUI::GameMainMenu>          (new GUI::GameMainMenu          ());
    m_impl->panel_GameSettings        = std::unique_ptr<GUI::GameSettings>          (new GUI::GameSettings          ());
    m_impl->panel_GameAbout           = std::unique_ptr<GUI::GameAbout>             (new GUI::GameAbout             ());
    m_impl->panel_SimUtils            = std::unique_ptr<GUI::SimUtils>              (new GUI::SimUtils              ());
    m_impl->panel_FrictionSettings    = std::unique_ptr<GUI::FrictionSettings>      (new GUI::FrictionSettings      ());
    m_impl->panel_MultiplayerSelector = std::unique_ptr<GUI::MultiplayerSelector>   (new GUI::MultiplayerSelector   ());
    m_impl->panel_MainSelector        = std::shared_ptr<GUI::MainSelector>          (new GUI::MainSelector          ());
    m_impl->panel_TopMenubar          = std::unique_ptr<GUI::TopMenubar>            (new GUI::TopMenubar            ());
}

// GUI SetVisible*()
#define SET_VISIBLE(_VAR_, _VIS_) { assert(m_impl); assert(m_impl->_VAR_.get()); m_impl->_VAR_->SetVisible(_VIS_); }

void GUIManager::SetVisible_GameMainMenu        (bool visible) { SET_VISIBLE(panel_GameMainMenu       , visible) }
void GUIManager::SetVisible_GameAbout           (bool visible) { SET_VISIBLE(panel_GameAbout          , visible) }
void GUIManager::SetVisible_GameSettings        (bool visible) { SET_VISIBLE(panel_GameSettings       , visible) }
void GUIManager::SetVisible_GamePauseMenu       (bool visible) { SET_VISIBLE(panel_GamePauseMenu      , visible) }
void GUIManager::SetVisible_DebugOptions        (bool visible) { SET_VISIBLE(panel_DebugOptions       , visible) }
void GUIManager::SetVisible_SimUtils            (bool visible) { SET_VISIBLE(panel_SimUtils           , visible) }
void GUIManager::SetVisible_MultiplayerSelector (bool visible) { SET_VISIBLE(panel_MultiplayerSelector, visible) }
void GUIManager::SetVisible_MainSelector        (bool visible) { SET_VISIBLE(panel_MainSelector       , visible) }
void GUIManager::SetVisible_ChatBox             (bool visible) { SET_VISIBLE(panel_ChatBox            , visible) }
void GUIManager::SetVisible_SpawnerReport       (bool visible) { SET_VISIBLE(panel_SpawnerReport      , visible) }
void GUIManager::SetVisible_VehicleDescription  (bool visible) { SET_VISIBLE(panel_VehicleDescription , visible) }
void GUIManager::SetVisible_MpClientList        (bool visible) { SET_VISIBLE(panel_MpClientList       , visible) }
void GUIManager::SetVisible_FrictionSettings    (bool visible) { SET_VISIBLE(panel_FrictionSettings   , visible) }
void GUIManager::SetVisible_TextureToolWindow   (bool visible) { SET_VISIBLE(panel_TextureToolWindow  , visible) }
void GUIManager::SetVisible_LoadingWindow       (bool visible) { SET_VISIBLE(panel_LoadingWindow      , visible) }
void GUIManager::SetVisible_TopMenubar          (bool visible) { SET_VISIBLE(panel_TopMenubar         , visible) }
void GUIManager::SetVisible_Console             (bool visible) { SET_VISIBLE(panel_GameConsole        , visible) }

// GUI IsVisible*()
#define IS_VISIBLE(_VAR_) assert(m_impl); assert(m_impl->_VAR_.get()); return m_impl->_VAR_->IsVisible();

bool GUIManager::IsVisible_GameMainMenu        () { IS_VISIBLE(panel_GameMainMenu       ) }
bool GUIManager::IsVisible_GameAbout           () { IS_VISIBLE(panel_GameAbout          ) }
bool GUIManager::IsVisible_GameSettings        () { IS_VISIBLE(panel_GameSettings       ) }
bool GUIManager::IsVisible_GamePauseMenu       () { IS_VISIBLE(panel_GamePauseMenu      ) }
bool GUIManager::IsVisible_DebugOptions        () { IS_VISIBLE(panel_DebugOptions       ) }
bool GUIManager::IsVisible_SimUtils            () { IS_VISIBLE(panel_SimUtils           ) }
bool GUIManager::IsVisible_MessageBox          () { IS_VISIBLE(panel_MessageBox         ) }
bool GUIManager::IsVisible_MultiplayerSelector () { IS_VISIBLE(panel_MultiplayerSelector) }
bool GUIManager::IsVisible_MainSelector        () { IS_VISIBLE(panel_MainSelector       ) }
bool GUIManager::IsVisible_ChatBox             () { IS_VISIBLE(panel_ChatBox            ) }
bool GUIManager::IsVisible_SpawnerReport       () { IS_VISIBLE(panel_SpawnerReport      ) }
bool GUIManager::IsVisible_VehicleDescription  () { IS_VISIBLE(panel_VehicleDescription ) }
bool GUIManager::IsVisible_MpClientList        () { IS_VISIBLE(panel_MpClientList       ) }
bool GUIManager::IsVisible_FrictionSettings    () { IS_VISIBLE(panel_FrictionSettings   ) }
bool GUIManager::IsVisible_TextureToolWindow   () { IS_VISIBLE(panel_TextureToolWindow  ) }
bool GUIManager::IsVisible_LoadingWindow       () { IS_VISIBLE(panel_LoadingWindow      ) }
bool GUIManager::IsVisible_TopMenubar          () { IS_VISIBLE(panel_TopMenubar         ) }
bool GUIManager::IsVisible_Console             () { IS_VISIBLE(panel_GameConsole        ) }

// GUI GetInstance*()
Console*                    GUIManager::GetConsole()           { return m_impl->panel_GameConsole         .get(); }
GUI::MainSelector*          GUIManager::GetMainSelector()      { return m_impl->panel_MainSelector        .get(); }
GUI::LoadingWindow*         GUIManager::GetLoadingWindow()     { return m_impl->panel_LoadingWindow       .get(); }
GUI::MpClientList*          GUIManager::GetMpClientList()      { return m_impl->panel_MpClientList        .get(); }
GUI::MultiplayerSelector*   GUIManager::GetMpSelector()        { return m_impl->panel_MultiplayerSelector .get(); }
GUI::FrictionSettings*      GUIManager::GetFrictionSettings()  { return m_impl->panel_FrictionSettings    .get(); }
GUI::SimUtils*              GUIManager::GetSimUtils()          { return m_impl->panel_SimUtils            .get(); }
GUI::TopMenubar*            GUIManager::GetTopMenubar()        { return m_impl->panel_TopMenubar          .get(); }

void GUIManager::UnfocusGui()
{
	MyGUI::InputManager::getInstance().resetKeyFocusWidget();
	MyGUI::InputManager::getInstance().resetMouseCaptureWidget();
}

void GUIManager::destroy()
{
	destroyGui();
}

void GUIManager::createGui()
{
	std::string gui_logfilename = Application::GetSysLogsDir() + PATH_SLASH + "MyGUI.log";
	m_impl->mygui_platform = new MyGUI::OgrePlatform();
	m_impl->mygui_platform->initialise(
        RoR::Application::GetOgreSubsystem()->GetRenderWindow(), 
        gEnv->sceneManager,
        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
        gui_logfilename); // use cache resource group so preview images are working
	m_impl->mygui = new MyGUI::Gui();

	// empty init
	m_impl->mygui->initialise("");

	// add layer factory
	MyGUI::FactoryManager::getInstance().registerFactory<MyGUI::RTTLayer>("Layer");
	
	// then load the actual config
	MyGUI::ResourceManager::getInstance().load(RESOURCE_FILENAME);

	MyGUI::ResourceManager::getInstance().load(LanguageEngine::getSingleton().getMyGUIFontConfigFilename());

	// move the mouse into the middle of the screen, assuming we start at the top left corner (0,0)
	MyGUI::InputManager::getInstance().injectMouseMove(RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getWidth()*0.5f, RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getHeight()*0.5f, 0);

	// now find that font texture and save it - for debugging purposes
	/*
	ResourceManager::ResourceMapIterator it = TextureManager::getSingleton().getResourceIterator();
	while (it.hasMoreElements())
	{
		ResourcePtr res = it.getNext();
		if (res->getName().find("TrueTypeFont") != String::npos)
		{
			Image image;
			TexturePtr tex = (TexturePtr)res;
			tex->convertToImage(image);
			image.save(res->getName() + ".png");
			LOG(">> saved TTF texture: " + res->getName());
		}
	}
	*/

	//MyGUI::PluginManager::getInstance().loadPlugin("Plugin_BerkeliumWidget.dll");
	MyGUI::PointerManager::getInstance().setVisible(true);
	//Console *c = RoR::Application::GetConsole();
	//if (c) c->resized();
}

void GUIManager::destroyGui()
{
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

void GUIManager::initSimUtils()
{
    m_impl->panel_SimUtils.reset();
}

void GUIManager::killSimUtils()
{
    m_impl->panel_SimUtils.reset();
}

bool GUIManager::frameStarted(const Ogre::FrameEvent& evt)
{
	if (m_renderwindow_closed) return false;
	if (!m_impl->mygui) return true;


	// now hide the mouse cursor if not used since a long time
	if (getLastMouseMoveTime() > 5000)
	{
		MyGUI::PointerManager::getInstance().setVisible(false);
		//RoR::Application::GetGuiManager()->GetTopMenubar()->setVisible(false);
	}

	return true;
}

bool GUIManager::frameEnded(const Ogre::FrameEvent& evt)
{
	return true;
};

void GUIManager::framestep(float dt)
{
	if (m_impl->panel_SimUtils) //Be sure that it exists
		m_impl->panel_SimUtils->framestep(dt);
};

void GUIManager::PushNotification(Ogre::String Title, Ogre::UTFString text)
{
	if (!m_impl->panel_SimUtils) return;

	m_impl->panel_SimUtils->PushNotification(Title, text);
}

void GUIManager::HideNotification()
{
	if (!m_impl->panel_SimUtils) return;

	m_impl->panel_SimUtils->HideNotification();
}

void GUIManager::windowResized(Ogre::RenderWindow* rw)
{
	int width = (int)rw->getWidth();
	int height = (int)rw->getHeight();
	setInputViewSize(width, height);

	BeamFactory *bf = BeamFactory::getSingletonPtr();
	if (bf) bf->windowResized();

	if (m_impl->panel_GameMainMenu.get() != nullptr)
	{
		/* Adjust menu position */
		Ogre::Viewport* viewport = RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getViewport(0);
		int margin = (viewport->getActualHeight() / 15);
		m_impl->panel_GameMainMenu->SetPosition(
			margin, // left
			viewport->getActualHeight() - m_impl->panel_GameMainMenu->GetHeight() - margin // top
			);
	}
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
    Ogre::Viewport* viewport = RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getViewport(0);
    int margin = (viewport->getActualHeight() / 15);
    int top = viewport->getActualHeight() - m_impl->panel_GameMainMenu->GetHeight() - margin;
    m_impl->panel_GameMainMenu->SetPosition(margin, top);
}

void GUIManager::UpdateSimUtils(float dt, Beam *truck)
{
	if (m_impl->is_sim_utils_visible) //Better to update only when it's visible.
	{
		if (m_impl->panel_SimUtils) //Be sure that it exists
			m_impl->panel_SimUtils->UpdateStats(dt, truck);
	}
}

void GUIManager::ShowMessageBox(Ogre::String mTitle, Ogre::String mText, bool button1, Ogre::String mButton1, bool AllowClose = false, bool button2 = false, Ogre::String mButton2 = "")
{
	if (m_impl->panel_MessageBox.get() == nullptr)
		m_impl->panel_MessageBox = std::unique_ptr<GUI::gMessageBox>(new GUI::gMessageBox());

	m_impl->panel_MessageBox->ShowMessageBox(mTitle, mText, button1, mButton1, AllowClose, button2, mButton2);
}

void GUIManager::UpdateMessageBox(Ogre::String mTitle, Ogre::String mText, bool button1, Ogre::String mButton1, bool AllowClose = false, bool button2 = false, Ogre::String mButton2 = "", bool IsVisible = true)
{
	if (m_impl->panel_MessageBox.get() == nullptr)
		m_impl->panel_MessageBox = std::unique_ptr<GUI::gMessageBox>(new GUI::gMessageBox());

	m_impl->panel_MessageBox->UpdateMessageBox(mTitle, mText, button1, mButton1, AllowClose, button2, mButton2, IsVisible);
}

int GUIManager::getMessageBoxResult()
{
	if (m_impl->panel_MessageBox.get() == nullptr)
		return 0;

	return m_impl->panel_MessageBox->getResult();
}

void GUIManager::InitMainSelector(RoR::SkinManager* skin_manager)
{
	if (m_impl->panel_MainSelector.get() == nullptr)
	{
		
	}
	else
	{
		assert(false && "ERROR: Trying to init MainSelector more than 1 time.");
		LOG("ERROR: Trying to init MainSelector more than 1 time.");
	}
}

void GUIManager::AdjustPauseMenuPosition()
{
	Ogre::Viewport* viewport = RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getViewport(0);
	int margin = (viewport->getActualHeight() / 15);
	m_impl->panel_GamePauseMenu->SetPosition(
		margin, // left
		viewport->getActualHeight() - m_impl->panel_GamePauseMenu->GetHeight() - margin // top
		);
}

void GUIManager::AddRigLoadingReport(std::string const & vehicle_name, std::string const & text, int num_errors, int num_warnings, int num_other)
{
	m_impl->panel_SpawnerReport->SetRigLoadingReport(vehicle_name, text, num_errors, num_warnings, num_other);
}

void GUIManager::CenterSpawnerReportWindow()
{
	m_impl->panel_SpawnerReport->CenterToScreen();
}

void GUIManager::pushMessageChatBox(Ogre::String txt)
{
	if (m_impl->panel_ChatBox.get() == nullptr)
		m_impl->panel_ChatBox = std::unique_ptr<GUI::GameChatBox>(new GUI::GameChatBox());

	m_impl->panel_ChatBox->pushMsg(txt);
}

void GUIManager::hideGUI(bool hidden)
{
	if (m_impl->panel_SimUtils)
	{
		if (hidden)
		{
			m_impl->panel_SimUtils->HideNotification();
			m_impl->panel_SimUtils->SetFPSBoxVisible(false);
			m_impl->panel_SimUtils->SetTruckInfoBoxVisible(false);
			if (m_impl->panel_ChatBox.get() != nullptr)
			{
				m_impl->panel_ChatBox->Hide();
			}
		}
		m_impl->panel_SimUtils->DisableNotifications(hidden);
	}
}

void GUIManager::FrictionSettingsUpdateCollisions()
{
    Application::GetGuiManager()->GetFrictionSettings()->setCollisions(gEnv->collisions);
}

} // namespace RoR
