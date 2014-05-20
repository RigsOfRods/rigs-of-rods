#include "GameState.h"

#include "Application.h"
#include "OgreSubsystem.h"
#include "RoRFrameListener.h"
#include "Settings.h"

#include <OgreRoot.h>

using namespace Ogre;

RoRFrameListener *mFrameListener = 0;

GameState::GameState()
{
}

void GameState::enter()
{
    LOG("Entering GameState...");

	m_pSceneMgr = RoR::Application::GetOgreSubsystem()->GetOgreRoot()->createSceneManager(ST_EXTERIOR_CLOSE);
	gEnv->sceneManager = m_pSceneMgr;

	//CREATE CAMERA
	LOG("Creating camera");
	// Create the camera
	m_pCamera = m_pSceneMgr->createCamera("PlayerCam");
	gEnv->mainCamera = m_pCamera;

	// Position it at 500 in Z direction
	m_pCamera->setPosition(Vector3(128,25,128));
	// Look back along -Z
	m_pCamera->lookAt(Vector3(0,0,-300));
	m_pCamera->setNearClipDistance( 0.5 );
	m_pCamera->setFarClipDistance( 1000.0*1.733 );
	m_pCamera->setFOVy(Degree(60));
	m_pCamera->setAutoAspectRatio(true);
	RoR::Application::GetOgreSubsystem()->GetViewport()->setCamera(m_pCamera);
	
	// TO BE DONE:
	//m_pSceneMgr->setCameraRelativeRendering(true);

	LOG("Adding Frame Listener");

	mFrameListener = new RoRFrameListener(this,	RoR::Application::GetOgreSubsystem()->GetMainHWND());
	gEnv->frameListener = mFrameListener;

	RoR::Application::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(mFrameListener);
}

bool GameState::pause()
{
    LOG("Pausing GameState...");

    return true;
}

void GameState::resume()
{
    LOG("Resuming GameState...");

    RoR::Application::GetOgreSubsystem()->GetViewport()->setCamera(m_pCamera);
}

void GameState::exit()
{
    LOG("Leaving GameState...");

	if (mFrameListener)
	{
		RoR::Application::GetOgreSubsystem()->GetOgreRoot()->removeFrameListener(mFrameListener);
		delete mFrameListener;
	}

    m_pSceneMgr->destroyCamera(m_pCamera);
    if (m_pSceneMgr)
        RoR::Application::GetOgreSubsystem()->GetOgreRoot()->destroySceneManager(m_pSceneMgr);
}

void GameState::createScene()
{
}

void GameState::update(double timeSinceLastFrame)
{
}

void GameState::resized(Ogre::RenderWindow* rw)
{
	mFrameListener->windowResized(rw);
}
