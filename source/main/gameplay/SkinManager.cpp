/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

#include "SkinManager.h"

#include "OgreSubsystem.h"
#include "Utils.h"

#include <OgreEntity.h>
#include <OgreMaterialManager.h>
#include <OgrePass.h>
#include <OgreSubEntity.h>
#include <OgreTechnique.h>

RoR::SkinManager::SkinManager() : ResourceManager()
{
    mLoadOrder = 200.0f;
    mScriptPatterns.push_back("*.skin");
    mResourceType = "RoRVehicleSkins";

    Ogre::ResourceGroupManager::getSingleton()._registerScriptLoader(this);
    Ogre::ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);
}

RoR::SkinManager::~SkinManager()
{
    Ogre::ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
    Ogre::ResourceGroupManager::getSingleton()._unregisterScriptLoader(this);
}

void RoR::SkinManager::parseScript(Ogre::DataStreamPtr& stream, const Ogre::String& groupName)
{
    SkinDef* skin_def = nullptr;
    bool     skin_is_new = true;
    try
    {
        while(!stream->eof())
        {
            std::string line = RoR::Utils::SanitizeUtf8String(stream->getLine());

            // Ignore blanks & comments
            if (!line.length() || line.substr(0, 2) == "//")
            {
                continue;
            }

            if (!skin_def)
            {
                // No current skin -- So first valid data should be skin name
                Ogre::StringUtil::trim(line);
                auto search = m_skins.find(line);
                if (search != m_skins.end())
                {
                    skin_def = search->second;
                    skin_is_new = false;
                }
                else
                {
                    skin_def = new SkinDef;
                    skin_def->name = line;
                    skin_is_new = true;
                }
                stream->skipLine("{");
            }
            else
            {
                // Already in skin
                if (line == "}")
                {
                    if (skin_is_new)
                        m_skins.insert(std::make_pair(skin_def->name, skin_def));
                    skin_def = nullptr;// Finished
                }
                else
                {
                    this->ParseSkinAttribute(line, skin_def);
                }
            }
        }
    }
    catch (Ogre::ItemIdentityException& e)
    {
        if (skin_def != nullptr)
            delete skin_def;
        // this catches duplicates -> to be ignored
        // this happens since we load the full skin data off the cache, so we don't need
        // to re-add it to the SkinManager
        return;
    }
}

void RoR::SkinManager::ParseSkinAttribute(const std::string& line, SkinDef* skin_def)
{
    Ogre::StringVector params = Ogre::StringUtil::split(line, "\t=,;\n");
    for (unsigned int i=0; i < params.size(); i++)
    {
        Ogre::StringUtil::trim(params[i]);
    }
    Ogre::String& attrib = params[0];
    Ogre::StringUtil::toLowerCase(attrib);

    if (attrib == "replacetexture"  && params.size() == 3) { skin_def->replace_textures.insert(std::make_pair(params[1], params[2])); return; }
    if (attrib == "replacematerial" && params.size() == 3) { skin_def->replace_materials.insert(std::make_pair(params[1], params[2])); return; }
    if (attrib == "preview"         && params.size() >= 2) { skin_def->thumbnail = params[1]; return; }
    if (attrib == "description"     && params.size() >= 2) { skin_def->description = params[1]; return; }
    if (attrib == "authorname"      && params.size() >= 2) { skin_def->author_name = params[1]; return; }
    if (attrib == "authorid"        && params.size() == 2) { skin_def->author_id = PARSEINT(params[1]); return; }
    if (attrib == "guid"            && params.size() >= 2) { skin_def->guid = params[1]; Ogre::StringUtil::trim(skin_def->guid); Ogre::StringUtil::toLowerCase(skin_def->guid); return; }
    if (attrib == "name"            && params.size() >= 2) { skin_def->name = params[1]; Ogre::StringUtil::trim(skin_def->name); return; }
}

void RoR::SkinManager::GetUsableSkins(std::string guid, std::vector<SkinDef *> &out_skins)
{
    Ogre::StringUtil::toLowerCase(guid);
    for (auto entry: m_skins)
    {
        if (entry.second->guid == guid) // GUID already trimmed and lowercase
            out_skins.push_back(entry.second);
    }
}

void RoR::SkinManager::ReplaceMaterialTextures(SkinDef* skin_def, std::string materialName) // Static
{
    const auto not_found = skin_def->replace_textures.end();
    Ogre::MaterialPtr mat = RoR::OgreSubsystem::GetMaterialByName(materialName);
    if (!mat.isNull())
    {
        for (int t = 0; t < mat->getNumTechniques(); t++)
        {
            Ogre::Technique* tech = mat->getTechnique(0);
            if (!tech)
                continue;
            for (int p = 0; p < tech->getNumPasses(); p++)
            {
                Ogre::Pass* pass = tech->getPass(p);
                if (!pass)
                    continue;
                for (int tu = 0; tu < pass->getNumTextureUnitStates(); tu++)
                {
                    Ogre::TextureUnitState* tus = pass->getTextureUnitState(tu);
                    if (!tus)
                        continue;

                    //if (tus->getTextureType() != TEX_TYPE_2D) continue; // only replace 2d images
                    // walk the frames, usually there is only one
                    for (unsigned int fr = 0; fr < tus->getNumFrames(); fr++)
                    {
                        Ogre::String textureName = tus->getFrameTextureName(fr);
                        std::map<Ogre::String, Ogre::String>::iterator it = skin_def->replace_textures.find(textureName);
                        if (it != not_found)
                        {
                            textureName = it->second; //getReplacementForTexture(textureName);
                            tus->setFrameTextureName(textureName, fr);
                        }
                    }
                }
            }
        }
    }
}

void RoR::SkinManager::ApplySkinTextureReplacements(RoR::SkinDef* skin_def, Ogre::Entity* e) // Static
{
    assert(e != nullptr);
    assert(skin_def != nullptr);

    const auto not_found = skin_def->replace_materials.end();

    for (int n = 0; n < (int)e->getNumSubEntities(); n++)
    {
        Ogre::SubEntity* sub_entity = e->getSubEntity(n);
        auto itor = skin_def->replace_materials.find(sub_entity->getMaterialName());
        if (itor == not_found) // Only apply _texture_ replacements if there are no _material_ replacements
        {
            SkinManager::ReplaceMaterialTextures(skin_def, sub_entity->getMaterialName());
        }
    }
}

//we wont unload skins once loaded!
void RoR::SkinManager::unload(const Ogre::String& name) {}
void RoR::SkinManager::unload(Ogre::ResourceHandle handle) {}
void RoR::SkinManager::unloadAll(bool reloadableOnly) {}
void RoR::SkinManager::unloadUnreferencedResources(bool reloadableOnly) {}
void RoR::SkinManager::remove(Ogre::ResourcePtr& r) {}
void RoR::SkinManager::remove(const Ogre::String& name) {}
void RoR::SkinManager::remove(Ogre::ResourceHandle handle) {}
void RoR::SkinManager::removeAll(void) {}
void RoR::SkinManager::reloadAll(bool reloadableOnly) {}
void RoR::SkinManager::reloadUnreferencedResources(bool reloadableOnly) {}
