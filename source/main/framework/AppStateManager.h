//|||||||||||||||||||||||||||||||||||||||||||||||

#ifndef APP_STATE_MANAGER_HPP
#define APP_STATE_MANAGER_HPP

//|||||||||||||||||||||||||||||||||||||||||||||||

#include "RoRPrerequisites.h"
#include "AppState.h"
#include <pthread.h>

//|||||||||||||||||||||||||||||||||||||||||||||||

class AppStateManager : public AppStateListener
{
public:
	typedef struct
	{
		Ogre::String name;
		AppState* state;
	} state_info;

	AppStateManager();
	~AppStateManager();

	void manageAppState(Ogre::String stateName, AppState* state);

	AppState* findByName(Ogre::String stateName);


	void update(double dt);

	void start(AppState* state);
	void changeAppState(AppState* state);
	bool pushAppState(AppState* state);
	void popAppState();
	void pauseAppState();
	void shutdown();
	void tryShutdown(); // used upon errors and such where the state (and thus locking) is not clear
	void pauseRendering();
	void resized(Ogre::RenderWindow* rw);
    void popAllAndPushAppState(AppState* state);

protected:
	void init(AppState *state);
	pthread_mutex_t lock;

	std::vector<AppState*>		m_ActiveStateStack;
	std::vector<state_info>		m_States;
	bool						m_bShutdown; // exits the program
	bool                        m_bNoRendering; // no more rendering in the main loop
};

//|||||||||||||||||||||||||||||||||||||||||||||||

#endif

//|||||||||||||||||||||||||||||||||||||||||||||||