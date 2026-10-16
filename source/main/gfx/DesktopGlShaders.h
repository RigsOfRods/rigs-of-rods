// This file is part of Rigs of Rods, licensed under GPL-3.0.
#pragma once

#include <OgreHighLevelGpuProgramManager.h>
#include <OgreRenderSystem.h>
#include <OgreRenderSystemCapabilities.h>

namespace RoR
{
// MyGUI 3.4 and the explicit UI programs share this desktop GLSL contract.
inline bool UsesDesktopGlShaders(Ogre::RenderSystem* render_system)
{
    const Ogre::RenderSystemCapabilities* caps = render_system->getCapabilities();
    return !caps->hasCapability(Ogre::RSC_FIXED_FUNCTION)
        && Ogre::HighLevelGpuProgramManager::getSingleton().isLanguageSupported("glsl")
        && caps->isShaderProfileSupported("glsl150");
}
}
