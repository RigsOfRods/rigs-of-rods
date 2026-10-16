// This file is part of Rigs of Rods, licensed under GPL-3.0.
#pragma once

#include <OgreHighLevelGpuProgramManager.h>
#include <OgreHighLevelGpuProgram.h>
#include <stdexcept>
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

// Programs belong to the resource manager for this renderer lifetime. Font
// texture rebuilds may reuse these names, but never replace unrelated programs.
inline Ogre::HighLevelGpuProgramPtr LoadDesktopGlProgram(const Ogre::String& name,
    const Ogre::String& group, Ogre::GpuProgramType type, const Ogre::String& source)
{
    auto& manager = Ogre::HighLevelGpuProgramManager::getSingleton();
    Ogre::HighLevelGpuProgramPtr program = manager.getByName(name, group);
    if (program)
    {
        if (program->getType() != type || program->getLanguage() != "glsl" || program->getSource() != source)
            throw std::runtime_error("Desktop GLSL UI program name is already in use: " + name);
    }
    else
    {
        program = manager.createProgram(name, group, "glsl", type);
        program->setSource(source);
    }
    program->load();
    if (program->hasCompileError() || !program->isSupported())
        throw std::runtime_error("Desktop GLSL UI program failed to load or is unsupported: " + name);
    return program;
}
}
