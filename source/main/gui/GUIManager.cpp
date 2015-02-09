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
	mResourceFileName("MyGUI_Core.xml")
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

void GUIManager::unfocus()
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
	Console *c = RoR::Application::GetConsole();
	if (c) c->resized();
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

void GUIManager::windowResized(Ogre::RenderWindow* rw)
{
	int width = (int)rw->getWidth();
	int height = (int)rw->getHeight();
	setInputViewSize(width, height);

	BeamFactory *bf = BeamFactory::getSingletonPtr();
	if (bf) bf->windowResized();

	Console *c = RoR::Application::GetConsole();
	if (c) c->resized();
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

void GUIManager::AddRigLoadingReport(std::string const & vehicle_name, std::string const & text, int num_errors, int num_warnings, int num_other)
{
	m_rig_spawner_report_window->SetRigLoadingReport(vehicle_name, text, num_errors, num_warnings, num_other);
}

void GUIManager::ShowRigSpawnerReportWindow()
{
	m_rig_spawner_report_window->CenterToScreen();
	m_rig_spawner_report_window->Show();
}

void GUIManager::HideRigSpawnerReportWindow()
{
	m_rig_spawner_report_window->Hide();
}
