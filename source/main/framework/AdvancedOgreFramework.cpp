//|||||||||||||||||||||||||||||||||||||||||||||||

#include "AdvancedOgreFramework.h"

#include "Settings.h"
#include "ErrorUtils.h"
#include "Language.h"
#include "RoRVersion.h"
#include "RoRWindowEventUtilities.h"
#include "Utils.h"

//|||||||||||||||||||||||||||||||||||||||||||||||

using namespace Ogre;

// this is the global instance of GlobalEnvironment that everyone uses
GlobalEnvironment *gEnv = new GlobalEnvironment();

//|||||||||||||||||||||||||||||||||||||||||||||||

OgreFramework::OgreFramework() : 
	hwnd(Ogre::String()), 
	mainhwnd(Ogre::String()), 
	name()
{
    m_pRoot			    = 0;
    m_pRenderWnd		= 0;
    m_pViewport			= 0;
    m_pTimer			= 0;
}

//|||||||||||||||||||||||||||||||||||||||||||||||

OgreFramework::~OgreFramework()
{
	LOG("Shutdown OGRE...");
}

//|||||||||||||||||||||||||||||||||||||||||||||||

bool OgreFramework::configure(void)
{
	// Show the configuration dialog and initialise the system
	// You can skip this and use root.restoreConfig() to load configuration
	// settings if you were sure there are valid ones saved in ogre.cfg
	bool useogreconfig = BSETTING("USE_OGRE_CONFIG", false);

	bool ok = false;
	if (useogreconfig)
	{
		ok = m_pRoot->showConfigDialog();
	}
	else
	{
		ok = m_pRoot->restoreConfig();
	}
	if (ok)
	{
		// If returned true, user clicked OK so initialise
		// Here we choose to let the system create a default rendering window by passing 'true'
		m_pRenderWnd = m_pRoot->initialise(true, "Rigs of Rods version " + String(ROR_VERSION_STRING));

		// set window icon correctly
		fixRenderWindowIcon(m_pRenderWnd);

		return true;
	}
	else
	{
		showError(_L("Configuration error"), _L("Run the RoRconfig program first."));
		exit(1);
	}
	return true;
}


bool OgreFramework::loadOgrePlugins(Ogre::String pluginsfile)
{
	Ogre::StringVector pluginList;
	Ogre::String pluginDir;
	Ogre::ConfigFile cfg;

	try
	{
		cfg.load( pluginsfile );
	}
	catch (Ogre::Exception)
	{
		Ogre::LogManager::getSingleton().logMessage(pluginsfile + " not found, automatic plugin loading disabled.");
		return false;
	}

	pluginDir = cfg.getSetting("PluginFolder"); // Ignored on Mac OS X, uses Resources/ directory
	pluginList = cfg.getMultiSetting("Plugin");

#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE && OGRE_PLATFORM != OGRE_PLATFORM_IPHONE
	if (pluginDir.empty())
	{
		// User didn't specify plugins folder, try current one
		pluginDir = ".";
	}
#endif

	char last_char = pluginDir[pluginDir.length()-1];
	if (last_char != '/' && last_char != '\\')
	{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		pluginDir += "\\";
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		pluginDir += "/";
#endif
	}

	for ( Ogre::StringVector::iterator it = pluginList.begin(); it != pluginList.end(); ++it )
	{
		Ogre::String pluginFilename = pluginDir + (*it);
		try
		{
			m_pRoot->loadPlugin(pluginFilename);
		} catch(Exception &e)
		{
			LOG("failed to load plugin: " + pluginFilename + ": " + e.getFullDescription());
		}
	}
	return true;
}

bool OgreFramework::initOgre(Ogre::String name, Ogre::String hwnd, Ogre::String mainhwnd)
{
	this->name     = name;
	this->hwnd     = hwnd;
	this->mainhwnd = mainhwnd;

	if (!SETTINGS.setupPaths())
		return false;

	// load RoR.cfg directly after setting up paths
	try
	{
		SETTINGS.loadSettings(SSETTING("Config Root", "")+"RoR.cfg");
	} catch(Ogre::Exception& e)
	{
		String url = "http://wiki.rigsofrods.com/index.php?title=Error_" + TOSTRING(e.getNumber())+"#"+e.getSource();
		showOgreWebError(_L("A fatal exception has occured!"), ANSI_TO_UTF(e.getFullDescription()), ANSI_TO_UTF(url));
		showStoredOgreWebErrors();
		exit(1);
	}


	String logFilename   = SSETTING("Log Path", "") + name + Ogre::String(".log");
	String pluginsConfig = SSETTING("plugins.cfg", "plugins.cfg");
	String ogreConfig    = SSETTING("ogre.cfg", "ogre.cfg");
    m_pRoot = new Ogre::Root("", ogreConfig, logFilename);

	// load plugins manually
	loadOgrePlugins(pluginsConfig);

	// configure RoR
	configure();

    m_pViewport = m_pRenderWnd->addViewport(0);

	gEnv->ogreRoot         = m_pRoot;
	gEnv->viewPort     = m_pViewport;
	gEnv->renderWindow = m_pRenderWnd;
	
    m_pViewport->setBackgroundColour(ColourValue(0.5f, 0.5f, 0.5f, 1.0f));

    m_pViewport->setCamera(0);
	m_pViewport->setBackgroundColour(ColourValue::Black);

    Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

    m_pTimer = new Ogre::Timer();
    m_pTimer->reset();

    m_pRenderWnd->setActive(true);

    return true;
}


void OgreFramework::resized(Ogre::Vector2 size)
{
	// trigger resizing of all sub-components
	RoRWindowEventUtilities::triggerResize(m_pRenderWnd);

	// Set the aspect ratio for the new size
	if (m_pViewport->getCamera())
		m_pViewport->getCamera()->setAspectRatio(Ogre::Real(size.x) / Ogre::Real(size.y));
}
//|||||||||||||||||||||||||||||||||||||||||||||||






