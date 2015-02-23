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
	@file   GUIManager.h
	@author based on the basemanager code from mygui common
*/

#include "GUIManager.h"

#include "Application.h"
#include "BeamFactory.h"
#include "Console.h"
#include "GUI_RigEditorMenubar.h"
#include "Language.h"
#include "OgreSubsystem.h"
#include "RoRWindowEventUtilities.h"
#include "RTTLayer.h"
#include "Settings.h"
#include "TerrainManager.h"

#include <MyGUI_OgrePlatform.h>

using namespace Ogre;
using namespace RoR;

GUIManager::GUIManager() :
	mExit(false),
	mGUI(nullptr),
	mPlatform(nullptr),
	mResourceFileName("MyGUI_Core.xml"),
	isSimUtilsVisible(false)
{
	create();
}

GUIManager::~GUIManager()
{
}

bool GUIManager::create()
{
	RoR::Application::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(this);
	RoRWindowEventUtilities::addWindowEventListener(RoR::Application::GetOgreSubsystem()->GetRenderWindow(), this);

	windowResized(RoR::Application::GetOgreSubsystem()->GetRenderWindow());
	createGui();
#ifdef _WIN32
	MyGUI::LanguageManager::getInstance().eventRequestTag = MyGUI::newDelegate(this, &GUIManager::eventRequestTag);
#endif // _WIN32

	// Create panels
	m_rig_spawner_report_window = std::unique_ptr<GUI::RigSpawnerReportWindow>(new GUI::RigSpawnerReportWindow(this));

	return true;
}

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
	String gui_logfilename = SSETTING("Log Path", "") + "mygui.log";
	mPlatform = new MyGUI::OgrePlatform();
	mPlatform->initialise(RoR::Application::GetOgreSubsystem()->GetRenderWindow(), gEnv->sceneManager, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, gui_logfilename); // use cache resource group so preview images are working
	mGUI = new MyGUI::Gui();

	// empty init
	mGUI->initialise("");

	// add layer factory
	MyGUI::FactoryManager::getInstance().registerFactory<MyGUI::RTTLayer>("Layer");
	
	// then load the actual config
	MyGUI::ResourceManager::getInstance().load(mResourceFileName);

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
	if (mGUI)
	{
		mGUI->shutdown();
		delete mGUI;
		mGUI = nullptr;
	}

	if (mPlatform)
	{
		mPlatform->shutdown();
		delete mPlatform;
		mPlatform = nullptr;
	}
}

void GUIManager::initSimUtils()
{
	if (m_gui_SimUtils.get() == nullptr)
		m_gui_SimUtils = std::unique_ptr<GUI::SimUtils>(new GUI::SimUtils());
}

void GUIManager::killSimUtils()
{
	if (m_gui_SimUtils.get() != nullptr)
	{
		//delete(m_gui_SimUtils.get());
		m_gui_SimUtils->~SimUtils();
		m_gui_SimUtils = nullptr;
	}	
}

bool GUIManager::frameStarted(const FrameEvent& evt)
{
	if (mExit) return false;
	if (!mGUI) return true;


	// now hide the mouse cursor if not used since a long time
	if (getLastMouseMoveTime() > 5000)
	{
		MyGUI::PointerManager::getInstance().setVisible(false);
		//GUI_MainMenu::getSingleton().setVisible(false);
	}

	return true;
}

bool GUIManager::frameEnded(const FrameEvent& evt)
{
	return true;
};

void GUIManager::framestep(float dt)
{
	if (m_gui_SimUtils) //Be sure that it exists
		m_gui_SimUtils->framestep(dt);
};

void GUIManager::PushNotification(String Title, String text)
{
	if (!m_gui_SimUtils) return;

	m_gui_SimUtils->PushNotification(Title, text);
}
void GUIManager::windowResized(Ogre::RenderWindow* rw)
{
	int width = (int)rw->getWidth();
	int height = (int)rw->getHeight();
	setInputViewSize(width, height);

	BeamFactory *bf = BeamFactory::getSingletonPtr();
	if (bf) bf->windowResized();

	if (m_gui_GameMainMenu.get() != nullptr)
	{
		/* Adjust menu position */
		Ogre::Viewport* viewport = RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getViewport(0);
		int margin = (viewport->getActualHeight() / 15);
		m_gui_GameMainMenu->SetPosition(
			margin, // left
			viewport->getActualHeight() - m_gui_GameMainMenu->GetHeight() - margin // top
			);
	}
}

void GUIManager::windowClosed(Ogre::RenderWindow* rw)
{
	mExit = true;
}

void GUIManager::eventRequestTag(const MyGUI::UString& _tag, MyGUI::UString& _result)
{
	_result = MyGUI::LanguageManager::getInstance().getTag(_tag);
}

String GUIManager::getRandomWallpaperImage()
{
	
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

void GUIManager::SetSceneManager(Ogre::SceneManager* scene_manager)
{
	mPlatform->getRenderManagerPtr()->setSceneManager(scene_manager);
}


// ============================================================================
// Interface functions
// ============================================================================


void GUIManager::ShowMainMenu(bool isVisible)
{
	if (isVisible == true)
	{
		if (m_gui_GameMainMenu.get() == nullptr)
			m_gui_GameMainMenu = std::unique_ptr<GUI::GameMainMenu>(new GUI::GameMainMenu());

		/* Adjust menu position */
		Ogre::Viewport* viewport = RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getViewport(0);
		int margin = (viewport->getActualHeight() / 15);
		m_gui_GameMainMenu->SetPosition(
			margin, // left
			viewport->getActualHeight() - m_gui_GameMainMenu->GetHeight() - margin // top
			);

		m_gui_GameMainMenu->Show();
	} else 
		m_gui_GameMainMenu->Hide();
}

void GUIManager::ShowSettingGui(bool isVisible)
{
	if (isVisible == true)
	{
		if (m_gui_GameSettings.get() == nullptr)
			m_gui_GameSettings = std::unique_ptr<GUI::GameSettings>(new GUI::GameSettings());

		m_gui_GameSettings->Show();
	} else
		m_gui_GameSettings->Hide();
}

void GUIManager::ShowAboutGUI(bool isVisible)
{
	if (isVisible == true)
	{
		if (m_gui_GameAbout.get() == nullptr)
			m_gui_GameAbout = std::unique_ptr<GUI::GameAbout>(new GUI::GameAbout());

		m_gui_GameAbout->Show();
	}
	else
		m_gui_GameAbout->Hide();
}

void GUIManager::ShowDebugOptionsGUI(bool isVisible)
{
	if (isVisible == true)
	{
		if (m_gui_DebugOptions.get() == nullptr)
			m_gui_DebugOptions = std::unique_ptr<GUI::DebugOptions>(new GUI::DebugOptions());

		m_gui_DebugOptions->Show();
	}
	else
		m_gui_DebugOptions->Hide();
}

void GUIManager::ToggleFPSBox()
{
	if (!isSimUtilsVisible) //We don't know, might be already init'ed
	{
		isSimUtilsVisible = true;
	}

	//Easy as pie!
	m_gui_SimUtils->ToggleFPSBox();
}

void GUIManager::ToggleTruckInfoBox()
{
	if (!isSimUtilsVisible) //We don't know, might be already init'ed
	{
		isSimUtilsVisible = true;
	}

	m_gui_SimUtils->ToggleTruckInfoBox();
}

void GUIManager::UpdateSimUtils(float dt, Beam *truck)
{
	if (isSimUtilsVisible) //Better to update only when it's visible.
	{
		if (m_gui_SimUtils) //Be sure that it exists
			m_gui_SimUtils->UpdateStats(dt, truck);
	}
}

void GUIManager::ShowMessageBox(Ogre::String mTitle, Ogre::String mText, bool button1, Ogre::String mButton1, bool AllowClose = false, bool button2 = false, Ogre::String mButton2 = "")
{
	if (m_gui_gMessageBox.get() == nullptr)
		m_gui_gMessageBox = std::unique_ptr<GUI::gMessageBox>(new GUI::gMessageBox());

	m_gui_gMessageBox->ShowMessageBox(mTitle, mText, button1, mButton1, AllowClose, button2, mButton2);
}

int GUIManager::getMessageBoxResult()
{
	if (m_gui_gMessageBox.get() == nullptr)
		return 0;

	return m_gui_gMessageBox->getResult();
}

void GUIManager::ShowMultiPlayerSelector(bool isVisible)
{
	if (isVisible == true)
	{
		if (m_gui_MultiplayerSelector.get() == nullptr)
			m_gui_MultiplayerSelector = std::unique_ptr<GUI::MultiplayerSelector>(new GUI::MultiplayerSelector());

		m_gui_MultiplayerSelector->Show();
	}
	else
		m_gui_MultiplayerSelector->Hide();
}

void GUIManager::initMainSelector()
{
	if (m_gui_MainSelector.get() == nullptr)
		m_gui_MainSelector = std::shared_ptr<GUI::MainSelector>(new GUI::MainSelector());
	else
		LOG("ERROR: Trying to init MainSelector more than 1 time.");
}

void GUIManager::TogglePauseMenu()
{
	if (m_gui_GamePauseMenu.get() == nullptr)
	{
		//First time, to remove flicker glitch
		m_gui_GamePauseMenu = std::unique_ptr<GUI::GamePauseMenu>(new GUI::GamePauseMenu());

		/* Adjust menu position */
		Ogre::Viewport* viewport = RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getViewport(0);
		int margin = (viewport->getActualHeight() / 15);
		m_gui_GamePauseMenu->SetPosition(
			margin, // left
			viewport->getActualHeight() - m_gui_GamePauseMenu->GetHeight() - margin // top
			);
		m_gui_GamePauseMenu->Show();
		return;
	}
			

	/* Adjust menu position */
	Ogre::Viewport* viewport = RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getViewport(0);
	int margin = (viewport->getActualHeight() / 15);
	m_gui_GamePauseMenu->SetPosition(
		margin, // left
		viewport->getActualHeight() - m_gui_GamePauseMenu->GetHeight() - margin // top
		);

	if (!m_gui_GamePauseMenu->mMainWidget->getVisible())
		m_gui_GamePauseMenu->Show();
	else
		m_gui_GamePauseMenu->Hide();
}

bool GUIManager::GetPauseMenuVisible()
{
	if (m_gui_GamePauseMenu.get() != nullptr)
		return m_gui_GamePauseMenu->mMainWidget->getVisible();
	else
		return false;
}

void GUIManager::AddRigLoadingReport(std::string const & vehicle_name, std::string const & text, int num_errors, int num_warnings, int num_other)
{
	m_rig_spawner_report_window->SetRigLoadingReport(vehicle_name, text, num_errors, num_warnings, num_other);
}

void GUIManager::ShowRigSpawnerReportWindow()
{
	if (BSETTING("AutoRigSpawnerReport", false))
	{
		m_rig_spawner_report_window->CenterToScreen();
		m_rig_spawner_report_window->Show();
	}
}

void GUIManager::HideRigSpawnerReportWindow()
{
	m_rig_spawner_report_window->Hide();
	UnfocusGui();
}

void GUIManager::ShowChatBox()
{
	//This should happen only when the user is going to write something
	if (m_gui_ChatBox.get() == nullptr)
		m_gui_ChatBox = std::unique_ptr<GUI::GameChatBox>(new GUI::GameChatBox());

	m_gui_ChatBox->Show();
}

void GUIManager::pushMessageChatBox(Ogre::String txt)
{
	if (m_gui_ChatBox.get() == nullptr)
		m_gui_ChatBox = std::unique_ptr<GUI::GameChatBox>(new GUI::GameChatBox());

	m_gui_ChatBox->pushMsg(txt);
}

void GUIManager::SetNetChat(ChatSystem *c)
{
	if (m_gui_ChatBox.get() == nullptr)
		m_gui_ChatBox = std::unique_ptr<GUI::GameChatBox>(new GUI::GameChatBox());

	m_gui_ChatBox->setNetChat(c);
}