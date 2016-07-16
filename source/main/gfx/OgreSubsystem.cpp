/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.org/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   OgreSubsystem.cpp
	@author Petr Ohlidal
	@date   05/2014
*/

#include "OgreSubsystem.h"

#include "Settings.h"
#include "ErrorUtils.h"
#include "Language.h"
#include "RoRVersion.h"
#include "RoRWindowEventUtilities.h"
#include "Utils.h"

#include <OgreRoot.h>
#include <OgreConfigFile.h>
#include <OgreRenderWindow.h>
#include <OgreTimer.h>

namespace RoR
{

OgreSubsystem::OgreSubsystem() : 
	m_ogre_root(nullptr),
	m_render_window(nullptr),
	m_viewport(nullptr),
	m_timer(nullptr)
{}

OgreSubsystem::~OgreSubsystem()
{
	LOG("Shutting down OGRE subsystem");
}

bool OgreSubsystem::Configure()
{
	// Show the configuration dialog and initialise the system
	// You can skip this and use root.restoreConfig() to load configuration
	// settings if you were sure there are valid ones saved in ogre.cfg
	bool use_ogre_config = BSETTING("USE_OGRE_CONFIG", false);

	bool ok = false;
	if (use_ogre_config)
	{
		ok = m_ogre_root->showConfigDialog();
	}
	else
	{
		ok = m_ogre_root->restoreConfig();
	}
	if (ok)
	{
		// If returned true, user clicked OK so initialise
		// Here we choose to let the system create a default rendering window by passing 'true'
		m_render_window = m_ogre_root->initialise(true, "Rigs of Rods version " + Ogre::String(ROR_VERSION_STRING));

		// set window icon correctly
		fixRenderWindowIcon(m_render_window);

		return true;
	}
	else
	{
		ErrorUtils::ShowError(_L("Configuration error"), _L("Run the RoRconfig program first."));
		exit(1);
	}
	return true;
}


bool OgreSubsystem::LoadOgrePlugins(Ogre::String const & pluginsfile)
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
			m_ogre_root->loadPlugin(pluginFilename);
		} 
		catch(Ogre::Exception &e)
		{
			LOG("failed to load plugin: " + pluginFilename + ": " + e.getFullDescription());
		}
	}
	return true;
}

bool OgreSubsystem::StartOgre(Ogre::String const & name, Ogre::String const & hwnd, Ogre::String const & mainhwnd)
{
	m_name = name;
	m_hwnd = hwnd;
	m_main_hwnd = mainhwnd;

	try
	{
		SETTINGS.loadSettings(SSETTING("Config Root", "")+"RoR.cfg");
	} 
	catch(Ogre::Exception& e)
	{
		Ogre::String url = "http://wiki.rigsofrods.org/index.php?title=Error_" + TOSTRING(e.getNumber())+"#"+e.getSource();
		ErrorUtils::ShowOgreWebError(_L("A fatal exception has occured!"), ANSI_TO_UTF(e.getFullDescription()), ANSI_TO_UTF(url));
		ErrorUtils::ShowStoredOgreWebErrors();
		exit(1);
	}

	Ogre::String logFilename   = SSETTING("Log Path", "") + name + Ogre::String(".log");
	Ogre::String pluginsConfig = SSETTING("plugins.cfg", "plugins.cfg");
	Ogre::String ogreConfig    = SSETTING("ogre.cfg", "ogre.cfg");
    m_ogre_root = new Ogre::Root("", ogreConfig, logFilename);

	// load plugins manually
	LoadOgrePlugins(pluginsConfig);

	// configure RoR
	Configure();

    m_viewport = m_render_window->addViewport(nullptr);
	
    m_viewport->setBackgroundColour(Ogre::ColourValue(0.5f, 0.5f, 0.5f, 1.0f));

    m_viewport->setCamera(nullptr);
	m_viewport->setBackgroundColour(Ogre::ColourValue::Black);

    Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

    m_timer = new Ogre::Timer();
    m_timer->reset();

    m_render_window->setActive(true);

    return true;
}


void OgreSubsystem::WindowResized(Ogre::Vector2 const & size)
{
	// trigger resizing of all sub-components
	RoRWindowEventUtilities::triggerResize(m_render_window);

	// Set the aspect ratio for the new size
	if (m_viewport->getCamera())
	{
		m_viewport->getCamera()->setAspectRatio(Ogre::Real(size.x) / Ogre::Real(size.y));
	}
}

unsigned long OgreSubsystem::GetTimeSinceStartup() 
{ 
	return m_timer->getMilliseconds();
}

} // namespace RoR
