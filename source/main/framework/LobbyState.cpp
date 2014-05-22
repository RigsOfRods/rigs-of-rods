#include "LobbyState.h"

#include "Application.h"
#include "OgreSubsystem.h"
#include "GUIManager.h"
#include "LobbyGUI.h"
#include "InputEngine.h"
#include "SceneMouse.h"

#include <OgreRoot.h>

using namespace Ogre;

LobbyState::LobbyState()
{
}

void LobbyState::enter()
{
	LOG("Entering LobbyState...");

	m_pSceneMgr = RoR::Application::GetOgreSubsystem()->GetOgreRoot()->createSceneManager(ST_INTERIOR);

	INPUTENGINE.setupDefault();

	//CREATE CAMERA
	LOG("Creating camera");
	// Create the camera
	m_pCamera = m_pSceneMgr->createCamera("PlayerCam");
	// Position it at 500 in Z direction
	m_pCamera->setPosition(Vector3(128,25,128));
	// Look back along -Z
	m_pCamera->lookAt(Vector3(0,0,-300));
	m_pCamera->setNearClipDistance( 0.5 );
	m_pCamera->setFarClipDistance( 1000.0*1.733 );
	m_pCamera->setFOVy(Degree(60));
	m_pCamera->setAutoAspectRatio(true);
	RoR::Application::GetOgreSubsystem()->GetViewport()->setCamera(m_pCamera);

#ifdef USE_MYGUI
	

	LobbyGUI::getSingleton().setVisible(true);

	resized(RoR::Application::GetOgreSubsystem()->GetRenderWindow());
#endif //USE_MYGUI
}

bool LobbyState::pause()
{
	LOG("Pausing LobbyState...");

	return true;
}

void LobbyState::resume()
{
	LOG("Resuming LobbyState...");
}

void LobbyState::exit()
{
	LOG("Leaving LobbyState...");
}

void LobbyState::createScene()
{
}

void LobbyState::update(double dt)
{
	INPUTENGINE.Capture();

#ifdef USE_MYGUI
	LobbyGUI::getSingleton().update(dt);
#endif //USE_MYGUI
}

void LobbyState::resized(Ogre::RenderWindow* rw)
{
	INPUTENGINE.windowResized(rw);
}
