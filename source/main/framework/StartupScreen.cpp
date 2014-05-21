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

#include "StartupScreen.h"

#include "Application.h"
#include "GuiManager.h"
#include "OgreSubsystem.h"

#include <OgreSceneManager.h>
#include <OgreRoot.h>
#include <OgreOverlayManager.h>
#include <OgreMaterial.h>

using namespace RoR;

StartupScreen::StartupScreen():
	m_scene_manager(nullptr),
	m_overlay(nullptr),
	m_camera(nullptr)
{
	
}

void StartupScreen::InitAndShow()
{
	// Setup rendering
	m_scene_manager = Application::GetOgreSubsystem()->GetOgreRoot()->createSceneManager(Ogre::ST_EXTERIOR_CLOSE);
	assert(m_scene_manager != nullptr);
	m_camera = m_scene_manager->createCamera("PlayerCam");
	assert(m_camera != nullptr);
	RoR::Application::GetOgreSubsystem()->GetViewport()->setCamera(m_camera);

	// Create rendering overlay
	Ogre::OverlayManager& overlay_manager = Ogre::OverlayManager::getSingleton();
	m_overlay = static_cast<Ogre::Overlay*>( overlay_manager.getByName("RoR/StartupScreen") );
	if (!m_overlay)
	{
		OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Cannot find loading overlay", "StartupScreen::InitAndShow");
	}

	// Set random wallpaper image
#ifdef USE_MYGUI
	Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName("RoR/StartupScreenWallpaper");
	Ogre::String wallpaper_texture_name = GUIManager::getRandomWallpaperImage(); // TODO: manage by class Application
	if (! wallpaper_texture_name.empty() && ! mat.isNull())
	{
		if (mat->getNumTechniques() > 0 && mat->getTechnique(0)->getNumPasses() > 0 && mat->getTechnique(0)->getPass(0)->getNumTextureUnitStates() > 0)
		{
			mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(wallpaper_texture_name);
		}
	}
#endif // USE_MYGUI

	m_overlay->show();
	
	m_scene_manager->clearSpecialCaseRenderQueues();
	m_scene_manager->addSpecialCaseRenderQueue(Ogre::RENDER_QUEUE_OVERLAY);
	m_scene_manager->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_INCLUDE);
}

void StartupScreen::HideAndRemove()
{
	m_overlay->hide();

	// Back to full rendering
	m_scene_manager->clearSpecialCaseRenderQueues();
	m_scene_manager->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE);
	
	m_scene_manager->destroyCamera(m_camera);

    RoR::Application::GetOgreSubsystem()->GetOgreRoot()->destroySceneManager(m_scene_manager);
}
