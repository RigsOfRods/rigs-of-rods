/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "RoRPrerequisites.h"

#include <Ogre.h>

/////////////////////////////////////////////////////////////////////////////
// TODO: Rename these classes! Danger of confusion with 'class DashBoard'. //
/////////////////////////////////////////////////////////////////////////////

class DashboardListener;

class Dashboard : public ZeroedMemoryAllocator
{
public:

    Dashboard();
    ~Dashboard();

    void setEnable(bool en);

private:

    DashboardListener* mDashboardListener;
    Ogre::Camera* mDashCam;
    Ogre::RenderTexture* rttTex;
};

class DashboardListener : public Ogre::RenderTargetListener, public ZeroedMemoryAllocator
{
    friend class Dashboard;
public:

    DashboardListener();

    void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
    void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);

private:

    Ogre::Overlay* blendOverlay;
    Ogre::Overlay* dashOverlay;
    Ogre::Overlay* fpsOverlay;
    Ogre::Overlay* needlesOverlay;
    bool consolevisible;
    bool fpsDisplayed;
};
