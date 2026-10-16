// This file is part of Rigs of Rods, licensed under GPL-3.0.
#pragma once

#include "DesktopGlShaders.h"

#include <OgreMaterialManager.h>
#include <OgreResourceGroupManager.h>
#include <OgreViewport.h>
#include <Bites/OgreSGTechniqueResolverListener.h>
#include <RTShaderSystem/OgreShaderGenerator.h>
#include <memory>
#include <stdexcept>

namespace RoR
{
// Own only this bootstrap's additions. Root, scene and viewport must still exist
// at cleanup; declare this before a native callback owner so callbacks detach first.
class DesktopGlShaderBootstrap
{
public:
    DesktopGlShaderBootstrap() = default;
    DesktopGlShaderBootstrap(const DesktopGlShaderBootstrap&) = delete;
    DesktopGlShaderBootstrap& operator=(const DesktopGlShaderBootstrap&) = delete;

    ~DesktopGlShaderBootstrap() noexcept
    {
        // Each step is independent so unwinding can never throw past main.
        if (m_viewport)
        {
            try { m_viewport->setMaterialScheme(m_previous_scheme); } catch (...) {}
        }
        if (m_listener_registered)
        {
            try { Ogre::MaterialManager::getSingleton().removeListener(m_listener.get()); }
            catch (...) { m_listener.release(); } // Do not leave a dangling registered listener.
        }
        m_listener.reset();
        if (m_generator && m_scene)
        {
            try { m_generator->removeSceneManager(m_scene); } catch (...) {}
        }
        if (m_generator)
        {
            try { Ogre::RTShader::ShaderGenerator::destroy(); } catch (...) {}
        }
        if (m_location_added)
        {
            try { Ogre::ResourceGroupManager::getSingleton().removeResourceLocation(
                m_resource_path, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME); }
            catch (...) {}
        }
    }

    void initialize(Ogre::RenderSystem* render_system, const Ogre::String& resource_path)
    {
        if (!UsesDesktopGlShaders(render_system))
        {
            if (render_system->getName().find("OpenGL 3") != Ogre::String::npos)
                throw std::runtime_error("OpenGL 3 UI bootstrap requires GLSL and the glsl150 profile without fixed-function rendering.");
            return;
        }
        if (Ogre::RTShader::ShaderGenerator::getSingletonPtr())
            throw std::runtime_error("Desktop GLSL bootstrap cannot own an existing RTShaderSystem generator.");

        auto& resources = Ogre::ResourceGroupManager::getSingleton();
        const Ogre::String& group = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
        m_resource_path = resource_path;
        if (!resources.resourceLocationExists(resource_path, group))
        {
            try { resources.addResourceLocation(resource_path, "FileSystem", group, false); }
            catch (const Ogre::Exception&)
            {
                throw std::runtime_error("Desktop GLSL bootstrap needs the matching OGRE RTShaderLib/GLSL files at " + resource_path);
            }
            m_location_added = true;
        }
        const char* sentinels[] = {"FFPLib_Common.glsl", "FFPLib_Transform.glsl",
            "FFPLib_Texturing.glsl", "FFPLib_Fog.glsl", "FFPLib_AlphaTest.glsl",
            "SGXLib_PerPixelLighting.glsl"};
        for (const char* shader : sentinels)
        {
            if (!resources.openResource(shader, group, nullptr, false))
                throw std::runtime_error("Desktop GLSL bootstrap is missing " + Ogre::String(shader)
                    + "; install the matching OGRE RTShaderLib/GLSL files at " + resource_path);
        }

        // OGRE 1.11 can leave its newly allocated singleton if initialize throws.
        // The preexisting-singleton check above makes even that partial state ours.
        try
        {
            const bool initialized = Ogre::RTShader::ShaderGenerator::initialize();
            m_generator = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
            if (!initialized)
                throw std::runtime_error("Desktop GLSL RTShaderSystem initialization failed.");
        }
        catch (...)
        {
            m_generator = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
            throw;
        }
        m_generator->setTargetLanguage("glsl");
        m_listener.reset(new OgreBites::SGTechniqueResolverListener(m_generator));
        Ogre::MaterialManager::getSingleton().addListener(m_listener.get());
        m_listener_registered = true;
    }

    void attachScene(Ogre::SceneManager* scene, Ogre::Viewport* viewport)
    {
        if (!m_generator)
            return;
        m_generator->addSceneManager(scene);
        m_scene = scene;
        m_previous_scheme = viewport->getMaterialScheme();
        m_viewport = viewport; // Record before changing the scheme (including exception paths).
        viewport->setMaterialScheme(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
    }

private:
    Ogre::RTShader::ShaderGenerator* m_generator = nullptr;
    std::unique_ptr<OgreBites::SGTechniqueResolverListener> m_listener;
    Ogre::SceneManager* m_scene = nullptr;
    Ogre::Viewport* m_viewport = nullptr;
    Ogre::String m_previous_scheme;
    Ogre::String m_resource_path;
    bool m_listener_registered = false;
    bool m_location_added = false;
};
}
