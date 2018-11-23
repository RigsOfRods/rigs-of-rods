/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2018 Petr Ohlidal

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

/// @file
/// @author Petr Ohlidal
/// @date   05/2014
/// @brief  OGRE engine wrapper.

#pragma once

#include "RoRPrerequisites.h"

#include <OgreMaterial.h>

namespace RoR {

class OgreSubsystem : public ZeroedMemoryAllocator
{
public:

    OgreSubsystem();
    ~OgreSubsystem();

    bool StartOgre(Ogre::String const & hwnd, Ogre::String const & mainhwnd);

    void WindowResized(Ogre::Vector2 const & size);

    Ogre::String GetMainHWND() 
    { 
        return m_main_hwnd; 
    }

    Ogre::Root* GetOgreRoot()
    {
        return m_ogre_root;
    }

    Ogre::RenderWindow* GetRenderWindow()
    {
        return m_render_window;
    }

    Ogre::Viewport* GetViewport()
    {
        return m_viewport;
    }

    void SetViewport(Ogre::Viewport* viewport)
    {
        m_viewport = viewport;
    }

private:

    Ogre::String        m_hwnd;
    Ogre::String        m_main_hwnd;

    Ogre::Root*         m_ogre_root;
    Ogre::RenderWindow* m_render_window;
    Ogre::Viewport*     m_viewport;

    bool Configure();
    bool LoadOgrePlugins(Ogre::String const & pluginsfile);
};

} // namespace RoR
