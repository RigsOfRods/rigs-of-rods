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
