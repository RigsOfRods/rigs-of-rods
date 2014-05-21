#include "GameState.h"

#include "Application.h"
#include "OgreSubsystem.h"
#include "Settings.h"
#include "RoRFrameListener.h"

#include <OgreRoot.h>

using namespace Ogre;



GameState::GameState()
{
}

void GameState::enter()
{
    
	

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
