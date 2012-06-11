#include "GameState.h"

#include "RoRFrameListener.h"
#include "Settings.h"

using namespace Ogre;

RoRFrameListener *mFrameListener = 0;

GameState::GameState()
{
}

void GameState::enter()
{
    LOG("Entering GameState...");

	m_pSceneMgr = OgreFramework::getSingletonPtr()->m_pRoot->createSceneManager(ST_EXTERIOR_CLOSE);
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
	OgreFramework::getSingletonPtr()->m_pViewport->setCamera(m_pCamera);
	
	// TO BE DONE:
	//m_pSceneMgr->setCameraRelativeRendering(true);

	LOG("Adding Frame Listener");

	mFrameListener = new RoRFrameListener(this,	OgreFramework::getSingleton().getMainHWND());
	gEnv->frameListener = mFrameListener;

	OgreFramework::getSingleton().m_pRoot->addFrameListener(mFrameListener);
}

bool GameState::pause()
{
    LOG("Pausing GameState...");

    return true;
}

void GameState::resume()
{
    LOG("Resuming GameState...");

    OgreFramework::getSingletonPtr()->m_pViewport->setCamera(m_pCamera);
}

void GameState::exit()
{
	if (BSETTING("REPO_MODE", false))
	{
		std::exit(1);
	}

    LOG("Leaving GameState...");

	if (mFrameListener)
	{
		OgreFramework::getSingleton().m_pRoot->removeFrameListener(mFrameListener);
		delete mFrameListener;
	}

    m_pSceneMgr->destroyCamera(m_pCamera);
    if (m_pSceneMgr)
        OgreFramework::getSingletonPtr()->m_pRoot->destroySceneManager(m_pSceneMgr);
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
