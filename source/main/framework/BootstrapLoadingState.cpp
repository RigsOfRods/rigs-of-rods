#include "BootstrapLoadingState.h"
#include "ContentManager.h"

using namespace Ogre;

BootstrapLoadingState::BootstrapLoadingState() : mLoadingBar(0)
{
}

void BootstrapLoadingState::enter()
{
    LOG("Entering DummyState...");
	m_pSceneMgr = OgreFramework::getSingletonPtr()->m_pRoot->createSceneManager(ST_EXTERIOR_CLOSE);
	m_pCamera = m_pSceneMgr->createCamera("PlayerCam");
	OgreFramework::getSingletonPtr()->m_pViewport->setCamera(m_pCamera);

	ContentManager::getSingleton().initBootstrap();

	LOG("creating loading bar");
	// load all resources now, so the zip files are also initiated
	mLoadingBar = new ExampleLoadingBar();
	if (mLoadingBar)
	{
		RenderWindow *win = OgreFramework::getSingleton().m_pRenderWnd;
		mLoadingBar->start(win, 8, 8, 0.75);
	}
	m_pSceneMgr->clearSpecialCaseRenderQueues();
	m_pSceneMgr->addSpecialCaseRenderQueue(RENDER_QUEUE_OVERLAY);
	m_pSceneMgr->setSpecialCaseRenderQueueMode(SceneManager::SCRQM_INCLUDE);
}

bool BootstrapLoadingState::pause()
{
    LOG("Pausing DummyState...");

    return true;
}

void BootstrapLoadingState::resume()
{
    LOG("Resuming DummyState...");

    OgreFramework::getSingletonPtr()->m_pViewport->setCamera(m_pCamera);
}

void BootstrapLoadingState::exit()
{
    LOG("Leaving DummyState...");

	// Back to full rendering
	m_pSceneMgr->clearSpecialCaseRenderQueues();
	m_pSceneMgr->setSpecialCaseRenderQueueMode(SceneManager::SCRQM_EXCLUDE);

	if (mLoadingBar)
	{
		mLoadingBar->finish();
		delete(mLoadingBar);
		mLoadingBar=NULL;
	}
	
	if (m_pCamera)
		m_pSceneMgr->destroyCamera(m_pCamera);

    if (m_pSceneMgr)
        OgreFramework::getSingletonPtr()->m_pRoot->destroySceneManager(m_pSceneMgr);

}

void BootstrapLoadingState::createScene()
{
}

void BootstrapLoadingState::update(double timeSinceLastFrame)
{
}

void BootstrapLoadingState::resized(Ogre::RenderWindow* rw)
{
}
