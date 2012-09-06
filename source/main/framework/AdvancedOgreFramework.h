//|||||||||||||||||||||||||||||||||||||||||||||||

#ifndef OGRE_FRAMEWORK_HPP
#define OGRE_FRAMEWORK_HPP

//|||||||||||||||||||||||||||||||||||||||||||||||

#include "RoRPrerequisites.h"

#include <OgreCamera.h>
#include <OgreEntity.h>
#include <OgreLogManager.h>
#include <OgreOverlay.h>
#include <OgreOverlayElement.h>
#include <OgreOverlayManager.h>
#include <OgreRoot.h>
#include <OgreViewport.h>
#include <OgreSceneManager.h>
#include <OgreRenderWindow.h>
#include <OgreConfigFile.h>

#include "Singleton.h"

//|||||||||||||||||||||||||||||||||||||||||||||||

class OgreFramework : public RoRSingleton<OgreFramework>, public ZeroedMemoryAllocator
{
public:
	OgreFramework();
	~OgreFramework();

	bool initOgre(Ogre::String name, Ogre::String hwnd, Ogre::String mainhwnd, bool embedded=false);
	void resized(Ogre::Vector2 size);

	Ogre::Root*		    m_pRoot;
	Ogre::RenderWindow*	m_pRenderWnd;
	Ogre::Viewport*		m_pViewport;
	Ogre::Timer*		m_pTimer;


	bool isEmbedded(void) { return embedded; };
	Ogre::String getMainHWND() { return mainhwnd; };

	unsigned long getTimeSinceStartup() { return m_pTimer->getMilliseconds(); };

private:
	bool embedded;
	OgreFramework(const OgreFramework&);
	OgreFramework& operator= (const OgreFramework&);

	Ogre::String hwnd, mainhwnd;
	Ogre::String name;

	bool configure();
	bool loadOgrePlugins(Ogre::String pluginsfile);
};

//|||||||||||||||||||||||||||||||||||||||||||||||

#endif

//|||||||||||||||||||||||||||||||||||||||||||||||