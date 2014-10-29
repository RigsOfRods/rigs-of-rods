//|||||||||||||||||||||||||||||||||||||||||||||||

#include "AppStateManager.h"

#include "RoRWindowEventUtilities.h"
#include "Language.h"

#include <OgreLogManager.h>

#include "Settings.h"
#include "Utils.h"

using namespace Ogre;

//|||||||||||||||||||||||||||||||||||||||||||||||

AppStateManager::AppStateManager()
{
	m_bShutdown = false;
	m_bNoRendering = false;
	pthread_mutex_init(&lock, NULL);
}

//|||||||||||||||||||||||||||||||||||||||||||||||

AppStateManager::~AppStateManager()
{
	state_info si;

    while(!m_ActiveStateStack.empty())
	{
		m_ActiveStateStack.back()->exit();
		m_ActiveStateStack.pop_back();
	}

	while(!m_States.empty())
	{
		si = m_States.back();
        si.state->destroy();
        m_States.pop_back();
	}
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void AppStateManager::manageAppState(Ogre::String stateName, AppState* state)
{
	try
	{
		state_info new_state_info;
		new_state_info.name = stateName;
		new_state_info.state = state;
		m_States.push_back(new_state_info);
	}
	catch(std::exception& e)
	{
		delete state;
		throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, _L("Error while trying to manage a new AppState\n") + Ogre::String(e.what()), "AppStateManager.cpp (39)");
	}
}

//|||||||||||||||||||||||||||||||||||||||||||||||

AppState* AppStateManager::findByName(Ogre::String stateName)
{
	std::vector<state_info>::iterator itr;

	for (itr=m_States.begin();itr!=m_States.end();itr++)
	{
		if (itr->name==stateName)
			return itr->state;
	}

	return 0;
}

//|||||||||||||||||||||||||||||||||||||||||||||||
void AppStateManager::update(double dt)
{
	MUTEX_LOCK(&lock);
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	RoRWindowEventUtilities::messagePump();
#endif
	Ogre::RenderWindow* rw = OgreFramework::getSingletonPtr()->m_pRenderWnd;
	if (rw->isClosed())
	{
		// unlock before shutdown
		MUTEX_UNLOCK(&lock);
		// shutdown locks the mutex itself
		shutdown();
		return;
	}

	m_ActiveStateStack.back()->update(dt);
	OgreFramework::getSingletonPtr()->m_pRoot->renderOneFrame();

	if (!rw->isActive() && rw->isVisible())
		rw->update(); // update even when in background !

	MUTEX_UNLOCK(&lock);
}

void AppStateManager::start(AppState* state)
{
	changeAppState(state);

	unsigned long timeSinceLastFrame = 1;
	unsigned long startTime          = 0;
	unsigned long minTimePerFrame    = 0;
	unsigned long fpsLimit           = ISETTING("FPS-Limiter", 0);

	if (fpsLimit < 10 || fpsLimit >= 200)
	{
		fpsLimit = 0;
	}

	if (fpsLimit)
	{
		minTimePerFrame = 1000 / fpsLimit;
	}

	while(!m_bShutdown)
	{
		startTime = OgreFramework::getSingletonPtr()->m_pTimer->getMilliseconds();

		// no more actual rendering?
		if (m_bNoRendering)
		{
			sleepMilliSeconds(100);
			continue;
		}

		update(timeSinceLastFrame);

		if (fpsLimit && timeSinceLastFrame < minTimePerFrame)
		{
			// Sleep twice as long as we were too fast.
			sleepMilliSeconds((minTimePerFrame - timeSinceLastFrame) << 1);
		}


		timeSinceLastFrame = OgreFramework::getSingletonPtr()->m_pTimer->getMilliseconds() - startTime;
	}
	LOG("Main loop quit");
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void AppStateManager::changeAppState(AppState* state)
{
	if (!m_ActiveStateStack.empty())
	{
		m_ActiveStateStack.back()->exit();
		m_ActiveStateStack.pop_back();
	}

	m_ActiveStateStack.push_back(state);
	init(state);
	m_ActiveStateStack.back()->enter();
}

//|||||||||||||||||||||||||||||||||||||||||||||||

bool AppStateManager::pushAppState(AppState* state)
{
	if (!m_ActiveStateStack.empty())
	{
		if (!m_ActiveStateStack.back()->pause())
			return false;
	}

	m_ActiveStateStack.push_back(state);
	init(state);
	m_ActiveStateStack.back()->enter();

	return true;
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void AppStateManager::popAppState()
{
	if (!m_ActiveStateStack.empty())
	{
		m_ActiveStateStack.back()->exit();
		m_ActiveStateStack.pop_back();
	}

	if (!m_ActiveStateStack.empty())
	{
		init(m_ActiveStateStack.back());
		m_ActiveStateStack.back()->resume();
	}
    else
		shutdown();
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void AppStateManager::popAllAndPushAppState(AppState* state)
{
    while(!m_ActiveStateStack.empty())
    {
        m_ActiveStateStack.back()->exit();
        m_ActiveStateStack.pop_back();
    }

    pushAppState(state);
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void AppStateManager::pauseAppState()
{
	if (!m_ActiveStateStack.empty())
	{
		m_ActiveStateStack.back()->pause();
	}

	if (m_ActiveStateStack.size() > 2)
	{
		init(m_ActiveStateStack.at(m_ActiveStateStack.size() - 2));
		m_ActiveStateStack.at(m_ActiveStateStack.size() - 2)->resume();
	}
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void AppStateManager::shutdown()
{
	// shutdown needs to be synced
	MUTEX_LOCK(&lock);
	m_bShutdown = true;
printf(">SH\n");
	MUTEX_UNLOCK(&lock);
}


void AppStateManager::tryShutdown()
{
	// important: we need trylock here, otherwise it could happen
	// that we run into a deadlock, when an exception is raised while the lock is hold.
	//
	pthread_mutex_trylock(&lock);
	m_bShutdown = true;
	MUTEX_UNLOCK(&lock);
}
//|||||||||||||||||||||||||||||||||||||||||||||||

void AppStateManager::pauseRendering()
{
	// shutdown needs to be synced
	MUTEX_LOCK(&lock);
	m_bNoRendering = true;
	MUTEX_UNLOCK(&lock);
}

//|||||||||||||||||||||||||||||||||||||||||||||||

void AppStateManager::init(AppState* state)
{

	OgreFramework::getSingletonPtr()->m_pRenderWnd->resetStatistics();
}

void AppStateManager::resized(Ogre::RenderWindow* rw)
{
	m_ActiveStateStack.back()->resized(rw);
}

//|||||||||||||||||||||||||||||||||||||||||||||||
