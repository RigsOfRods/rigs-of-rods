//|||||||||||||||||||||||||||||||||||||||||||||||

#ifndef APP_STATE_HPP
#define APP_STATE_HPP

//|||||||||||||||||||||||||||||||||||||||||||||||

#include "RoRPrerequisites.h"

#include "AdvancedOgreFramework.h"


//|||||||||||||||||||||||||||||||||||||||||||||||

class AppStateListener : public ZeroedMemoryAllocator
{
public:
	AppStateListener(){};
	virtual ~AppStateListener(){};

	virtual void manageAppState(Ogre::String stateName, AppState* state) = 0;

	virtual AppState*	findByName(Ogre::String stateName) = 0;
	virtual void		changeAppState(AppState *state) = 0;
	virtual bool		pushAppState(AppState* state) = 0;
	virtual void		popAppState() = 0;
	virtual void		pauseAppState() = 0;
	virtual void		shutdown() = 0;
    virtual void        popAllAndPushAppState(AppState* state) = 0;
	virtual void        resized(Ogre::RenderWindow* rw) = 0;
};

//|||||||||||||||||||||||||||||||||||||||||||||||

class AppState : public ZeroedMemoryAllocator
{
public:
	static void create(AppStateListener* parent, const Ogre::String name){};

	void destroy(){delete this;}

	virtual void enter() = 0;
	virtual void exit() = 0;
	virtual bool pause(){return true;}
	virtual void resume(){};
	virtual void update(double timeSinceLastFrame) = 0;
	virtual void resized(Ogre::RenderWindow* rw) = 0;

protected:
	AppState(){};

	AppState*	findByName(Ogre::String stateName){return m_pParent->findByName(stateName);}
	void		changeAppState(AppState* state){m_pParent->changeAppState(state);}
	bool		pushAppState(AppState* state){return m_pParent->pushAppState(state);}
	void		popAppState(){m_pParent->popAppState();}
	void		shutdown(){m_pParent->shutdown();}
    void        popAllAndPushAppState(AppState* state){m_pParent->popAllAndPushAppState(state);}

	AppStateListener*			m_pParent;

	Ogre::Camera*				m_pCamera;
	Ogre::SceneManager*			m_pSceneMgr;
    Ogre::FrameEvent            m_FrameEvent;
};

//|||||||||||||||||||||||||||||||||||||||||||||||

#define DECLARE_APPSTATE_CLASS(T)										\
static void create(AppStateListener* parent, const Ogre::String name)	\
{																		\
	T* myAppState = new T();											\
	myAppState->m_pParent = parent;										\
	parent->manageAppState(name, myAppState);							\
}

//|||||||||||||||||||||||||||||||||||||||||||||||

#endif

//|||||||||||||||||||||||||||||||||||||||||||||||