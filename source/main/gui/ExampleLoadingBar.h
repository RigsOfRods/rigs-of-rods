/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2009 Torus Knot Software Ltd
Also see acknowledgements in Readme.html

You may use this sample code for anything you like, it is not covered by the
same license as the rest of the engine.
-----------------------------------------------------------------------------
*/
/*
-----------------------------------------------------------------------------
Filename:    ExampleLoadingBar.h
Description: Defines an example loading progress bar which you can use during
startup, level changes etc to display loading progress.
IMPORTANT: Note that this progress bar relies on you having the OgreCore.zip
package already added to a resource group called 'Bootstrap' - this provides
the basic resources required for the progress bar and will be loaded automatically.
*/
#include "OgreResourceGroupManager.h"
#include "OgreException.h"
#include "OgreOverlay.h"
#include "OgreOverlayManager.h"
#include "OgreRenderWindow.h"

#include "RoRWindowEventUtilities.h"

#include "GUIManager.h"
#include "Language.h"

using namespace Ogre;

/** Defines an example loading progress bar which you can use during
	startup, level changes etc to display loading progress.
@remarks
	Basically you just need to create an instance of this class, call start()
	before loading and finish() afterwards. You may also need to stop areas of
	your scene rendering in between since this method will call
	RenderWindow::update() to update the display of the bar - we advise using
	SceneManager's 'special case render queues' for this, see
	SceneManager::addSpecialCaseRenderQueue for details.
@note
	This progress bar relies on you having the OgreCore.zip package already
	added to a resource group called 'Bootstrap' - this provides the basic
	resources required for the progress bar and will be loaded automatically.
*/
class ExampleLoadingBar : public ResourceGroupListener, public ZeroedMemoryAllocator
{
protected:
	RenderWindow* mWindow;
	Overlay* mLoadOverlay;
	Real mInitProportion;
	unsigned short mNumGroupsInit;
	unsigned short mNumGroupsLoad;
	Real mProgressBarMaxSize;
	Real mProgressBarScriptSize;
	Real mProgressBarInc;
	OverlayElement* mLoadingBarElement;
	OverlayElement* mLoadingDescriptionElement;
	OverlayElement* mLoadingCommentElement;
	Ogre::Timer *t;
	int counterGroups;

public:
	ExampleLoadingBar() :
	    mWindow(NULL)
	  , mLoadOverlay(NULL)
	  , mInitProportion(1)
	  , mNumGroupsInit(0)
	  , mNumGroupsLoad(0)
	  , mProgressBarMaxSize(1)
	  , mProgressBarScriptSize(1)
	  , mProgressBarInc(1)
	  , mLoadingBarElement(NULL)
	  , mLoadingDescriptionElement(NULL)
	  , mLoadingCommentElement(0)
	  , t(NULL)
	  , counterGroups(0)
	{
		t = new Ogre::Timer();
	}
	
	virtual ~ExampleLoadingBar()
	{
		delete(t);
		t=NULL;
	}

	/** Show the loading bar and start listening.
	@param window The window to update
	@param numGroupsInit The number of groups you're going to be initialising
	@param numGroupsLoad The number of groups you're going to be loading
	@param initProportion The proportion of the progress which will be taken
		up by initialisation (ie script parsing etc). Defaults to 0.7 since
		script parsing can often take the majority of the time.
	*/
	virtual void start(RenderWindow* window,
		unsigned short numGroupsInit = 1,
		unsigned short numGroupsLoad = 1,
		Real initProportion = 0.70f)
	{
		mWindow = window;
		mNumGroupsInit = numGroupsInit;
		mNumGroupsLoad = numGroupsLoad;
		mInitProportion = initProportion;
		// We need to pre-initialise the 'Bootstrap' group so we can use
		// the basic contents in the loading screen
		ResourceGroupManager::getSingleton().initialiseResourceGroup("Bootstrap");

		OverlayManager& omgr = OverlayManager::getSingleton();
		mLoadOverlay = (Overlay*)omgr.getByName("Core/LoadOverlay");
		if (!mLoadOverlay)
		{
			OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
				"Cannot find loading overlay", "ExampleLoadingBar::start");
		}

		// use random wallpaper image
#ifdef USE_MYGUI
		MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName("Core/RoRLoadingScreen");
		String randomWallpaper = GUIManager::getRandomWallpaperImage();
		if (!randomWallpaper.empty() && !mat.isNull())
		{
			if (mat->getNumTechniques() > 0 && mat->getTechnique(0)->getNumPasses() > 0 && mat->getTechnique(0)->getPass(0)->getNumTextureUnitStates() > 0)
			{
				mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(randomWallpaper);
			}
		}
#endif //USE_MYGUI

		mLoadOverlay->show();

		// Save links to the bar and to the loading text, for updates as we go
		mLoadingBarElement = omgr.getOverlayElement("Core/LoadPanel/Bar/Progress");
		mLoadingCommentElement = omgr.getOverlayElement("Core/LoadPanel/Comment");
		//mLoadingCommentElement->setCaption(_L("Initialising..."));
		mLoadingDescriptionElement = omgr.getOverlayElement("Core/LoadPanel/Description");
		//mLoadingDescriptionElement->setCaption(_L("Loading, please wait..."));

		// we cannot use the translated strings as we havent loaded the languages yet

		OverlayElement* barContainer = omgr.getOverlayElement("Core/LoadPanel/Bar");
		mProgressBarMaxSize = barContainer->getWidth();
		mLoadingBarElement->setWidth(0);

		frameUpdate(true);

		// self is listener
		ResourceGroupManager::getSingleton().addResourceGroupListener(this);
	}

	/** Hide the loading bar and stop listening.
	*/
	virtual void finish(void)
	{
		// hide loading screen
		mLoadOverlay->hide();

		// Unregister listener
		ResourceGroupManager::getSingleton().removeResourceGroupListener(this);

	}

	void frameUpdate(bool force = false)
	{
		if (t->getMilliseconds() > 200 || force)
		{
			mWindow->update();
			RoRWindowEventUtilities::messagePump();
			t->reset();
		}
	}

	// ResourceGroupListener callbacks
	void resourceGroupScriptingStarted(const String& groupName, size_t scriptCount)
	{
		mProgressBarInc = mProgressBarMaxSize * mInitProportion / (Real)scriptCount;
		mProgressBarInc /= mNumGroupsInit;
		mLoadingDescriptionElement->setCaption(_L("Parsing scripts..."));

		counterGroups++;
		frameUpdate();
	}
	void scriptParseStarted(const String& scriptName, bool &skipThisScript)
	{
		mLoadingCommentElement->setCaption(scriptName);
		frameUpdate();
	}
	void scriptParseEnded(const String& scriptName, bool skipped)
	{
		mLoadingBarElement->setWidth(
			mLoadingBarElement->getWidth() + mProgressBarInc);
		frameUpdate();
	}
	void resourceGroupScriptingEnded(const String& groupName)
	{
	}
	void resourceGroupLoadStarted(const String& groupName, size_t resourceCount)
	{
		mProgressBarInc = mProgressBarMaxSize * (1-mInitProportion) / (Real)resourceCount;
		mProgressBarInc /= mNumGroupsLoad;
		mLoadingDescriptionElement->setCaption(_L("Loading resources..."));
		frameUpdate();
	}
	void resourceLoadStarted(const ResourcePtr& resource)
	{
		mLoadingCommentElement->setCaption(resource->getName());
		frameUpdate();
	}
	
	void resourceLoadEnded(void)
	{
	}
	
	void worldGeometryStageStarted(const String& description)
	{
		mLoadingCommentElement->setCaption(description);
		frameUpdate();
	}
	void worldGeometryStageEnded(void)
	{
		mLoadingBarElement->setWidth(
			mLoadingBarElement->getWidth() + mProgressBarInc);
		frameUpdate();
	}
	void resourceGroupLoadEnded(const String& groupName)
	{
	}

	Ogre::DataStreamPtr resourceLoading(const String &name, const String &group, Resource *resource)
	{
		return Ogre::DataStreamPtr();
	}

	bool resourceCollision(ResourcePtr &resource, ResourceManager *resourceManager)
	{
		return false;
	}

};

