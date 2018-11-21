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


/// @file   OgreSubsystem.cpp
/// @author Petr Ohlidal
/// @date   05/2014


#include "OgreSubsystem.h"

#include "Application.h"
#include "ErrorUtils.h"
#include "Language.h"
#include "PlatformUtils.h"
#include "RoRVersion.h"
#include "Settings.h"
#include "Utils.h"

#include <OgreCamera.h>
#include <OgreConfigFile.h>
#include <OgreMaterialManager.h>
#include <OgreRenderWindow.h>
#include <OgreRoot.h>
#include <OgreTextureManager.h>
#include <OgreTimer.h>
#include <OgreViewport.h>

namespace RoR
{

OgreSubsystem::OgreSubsystem() : 
    m_ogre_root(nullptr),
    m_render_window(nullptr),
    m_viewport(nullptr)
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
        ok = m_ogre_root->showConfigDialog(NULL);
    }
    else
    {
        ok = m_ogre_root->restoreConfig();
    }
    if (ok)
    {
        // If returned true, user clicked OK so initialise
        // Here we choose to let the system create a default rendering window by passing 'true'
        m_render_window = m_ogre_root->initialise(false);

        Ogre::ConfigOptionMap ropts = m_ogre_root->getRenderSystem ()->getConfigOptions ();
        Ogre::uint32 width, height;
        Ogre::NameValuePairList miscParams;

        std::istringstream mode (ropts["Video Mode"].currentValue);
        Ogre::String token;
        mode >> width; 
        mode >> token; // 'x' as seperator between width and height
        mode >> height;

        miscParams["FSAA"] = ropts["FSAA"].currentValue;
        miscParams["vsync"] = ropts["VSync"].currentValue;
        miscParams["gamma"] = ropts["sRGB Gamma Conversion"].currentValue;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        miscParams["windowProc"] = Ogre::StringConverter::toString ((size_t)OgreBites::WindowEventUtilities::_WndProc);
#endif

        m_render_window = Ogre::Root::getSingleton ().createRenderWindow (
            "Rigs of Rods version " + Ogre::String (ROR_VERSION_STRING),
            width, height, ropts["Full Screen"].currentValue == "Yes", &miscParams);
        OgreBites::WindowEventUtilities::_addRenderWindow (m_render_window);

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
    Ogre::ConfigFile cfg;

    try
    {
        cfg.load( pluginsfile );
    }
    catch (Ogre::Exception& e)
    {
        Ogre::LogManager::getSingleton().logMessage(pluginsfile + " not found, automatic plugin loading disabled. Message: " + e.getFullDescription());
        return false;
    }

    Ogre::String pluginDir = cfg.getSetting("PluginFolder");
    Ogre::StringVector pluginList = cfg.getMultiSetting("Plugin");

    if (pluginDir.empty())
    {
        pluginDir = RoR::App::sys_process_dir.GetActive();
    }

    for ( Ogre::StringVector::iterator it = pluginList.begin(); it != pluginList.end(); ++it )
    {
        Ogre::String pluginFilename = pluginDir + RoR::PATH_SLASH + (*it);
        try
        {
            m_ogre_root->loadPlugin(pluginFilename);
        }
        catch (Ogre::Exception& e)
        {
            LOG("failed to load plugin: " + pluginFilename + ": " + e.getFullDescription());
        }
    }
    return true;
}

bool OgreSubsystem::StartOgre(Ogre::String const & hwnd, Ogre::String const & mainhwnd)
{
    m_hwnd = hwnd;
    m_main_hwnd = mainhwnd;

    std::string log_filepath = std::string(RoR::App::sys_logs_dir.GetActive())   + RoR::PATH_SLASH + "RoR.log";
    std::string cfg_filepath = std::string(RoR::App::sys_config_dir.GetActive()) + RoR::PATH_SLASH + "ogre.cfg";
    m_ogre_root = new Ogre::Root("", cfg_filepath, log_filepath);

    // load plugins manually
    Str<300> plugins_path;
    plugins_path << RoR::App::sys_process_dir.GetActive() << RoR::PATH_SLASH << "plugins.cfg";
    this->LoadOgrePlugins(std::string(plugins_path));

    // configure RoR
    this->Configure();

    m_viewport = m_render_window->addViewport(nullptr);
    
    m_viewport->setBackgroundColour(Ogre::ColourValue(0.5f, 0.5f, 0.5f, 1.0f));

    m_viewport->setCamera(nullptr);
    m_viewport->setBackgroundColour(Ogre::ColourValue::Black);

    Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

    m_render_window->setActive(true);

    return true;
}


void OgreSubsystem::WindowResized(Ogre::Vector2 const & size)
{
    // Set the aspect ratio for the new size
    if (m_viewport->getCamera())
    {
        m_viewport->getCamera()->setAspectRatio(Ogre::Real(size.x) / Ogre::Real(size.y));
    }
}

// static
Ogre::MaterialPtr OgreSubsystem::GetMaterialByName(std::string const & name)
{
    // .getByName() returns Ogre::MaterialPtr under v1.9, but Ogre::ResourcePtr under v1.8
    return static_cast<Ogre::MaterialPtr>(Ogre::MaterialManager::getSingleton().getByName(name));
}

} // namespace RoR
