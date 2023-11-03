/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2023 Petr Ohlidal

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

#include "AddonPartFileFormat.h"

#include "Application.h"
#include "CacheSystem.h"
#include "Console.h"
#include "GenericFileFormat.h"
#include "RigDef_Parser.h"
#include "TuneupFileFormat.h"

#include <Ogre.h>

using namespace RoR;
using namespace RigDef;

std::shared_ptr<Document::Module> AddonPartUtility::TransformToRigDefModule(CacheEntryPtr& entry)
{
    App::GetCacheSystem()->LoadResource(entry);
    m_addonpart_entry = entry;
    try
    {
        Ogre::DataStreamPtr datastream = Ogre::ResourceGroupManager::getSingleton().openResource(entry->fname, entry->resource_group);

        m_document = new GenericDocument();
        BitMask_t options = GenericDocument::OPTION_ALLOW_SLASH_COMMENTS | GenericDocument::OPTION_ALLOW_NAKED_STRINGS;
        m_document->loadFromDataStream(datastream, options);

        m_module = std::shared_ptr<Document::Module>(new Document::Module(entry->dname));
        m_module->origin_addonpart = entry;
        m_context = new GenericDocContext(m_document);
        Keyword keyword = Keyword::INVALID;
        Keyword block = Keyword::INVALID;

        while (!m_context->endOfFile())
        {
            // (ignore 'addonpart_*' directives)
            if (m_context->isTokKeyword() && m_context->getTokKeyword().find("addonpart_") != std::string::npos)
            {
                m_context->seekNextLine();
                continue;
            }
            // Evaluate block 
            else if (m_context->isTokKeyword())
            {
                keyword = Parser::IdentifyKeyword(m_context->getTokKeyword());
                if (keyword == Keyword::MANAGEDMATERIALS ||
                    keyword == Keyword::PROPS ||
                    keyword == Keyword::FLEXBODIES)
                {
                    block = keyword;
                    m_context->seekNextLine();
                    continue; // !! go to next line
                }
            }

            // Process data in block
            switch (block)
            {
            case Keyword::MANAGEDMATERIALS: this->ProcessManagedMaterial(); break;
            case Keyword::PROPS: this->ProcessProp(); break;
            case Keyword::FLEXBODIES: this->ProcessFlexbody(); break;
            default: break;
            }

            m_context->seekNextLine();
        }    
        return m_module;
    }
    catch (Ogre::Exception& e)
    {
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Could not use addonpart: Error parsing file '{}', message: {}",
                entry->fname, e.getFullDescription()));
        return nullptr;
    }
}

void AddonPartUtility::ResolveUnwantedElements(TuneupDefPtr& tuneup, CacheEntryPtr& addonpart_entry)
{
    // Evaluates 'addonpart_unwanted_*' elements, respecting 'protected_*' directives in the tuneup.
    // ---------------------------------------------------------------------------------------------
    App::GetCacheSystem()->LoadResource(addonpart_entry);
    m_addonpart_entry = addonpart_entry;

    try
    {
        Ogre::DataStreamPtr datastream = Ogre::ResourceGroupManager::getSingleton().openResource(addonpart_entry->fname, addonpart_entry->resource_group);

        m_document = new GenericDocument();
        BitMask_t options = GenericDocument::OPTION_ALLOW_SLASH_COMMENTS | GenericDocument::OPTION_ALLOW_NAKED_STRINGS;
        m_document->loadFromDataStream(datastream, options);
        m_context = new GenericDocContext(m_document);

        while (!m_context->endOfFile())
        {
            if (m_context->isTokKeyword(0) 
                && m_context->getTokKeyword() == "addonpart_unwanted_prop" 
                && m_context->isTokString(1))
            {
                if (!tuneup->isPropProtected(m_context->getTokString(1)))
                    tuneup->remove_props.insert(m_context->getTokString(1));
            }
            else if (m_context->isTokKeyword(0) 
                && m_context->getTokKeyword() == "addonpart_unwanted_flexbody" 
                && m_context->isTokString(1))
            {
                if (!tuneup->isFlexbodyProtected(m_context->getTokString(1)))
                    tuneup->remove_flexbodies.insert(m_context->getTokString(1));
            }

            m_context->seekNextLine();
        }

    }
    catch (Ogre::Exception& e)
    {
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Addonpart unwanted elements check: Error parsing file '{}', message: {}",
                addonpart_entry->fname, e.getFullDescription()));
    }
}


// Helpers of `TransformToRigDefModule()`, they expect `m_context` to be in position:
// These expect `m_context` to be in position:

void AddonPartUtility::ProcessManagedMaterial()
{
    ManagedMaterial def;
    int n = m_context->countLineArgs();
    
    // Name:
    def.name = m_context->getStringData(0); // It may be a STRING (if with quotes), or KEYWORD (if without quotes - because it's at start of line).

    // Type:
    std::string str = m_context->getTokString(1);
    if (str == "mesh_standard")        def.type = ManagedMaterialType::MESH_STANDARD;
    if (str == "mesh_transparent")     def.type = ManagedMaterialType::MESH_TRANSPARENT;
    if (str == "flexmesh_standard")    def.type = ManagedMaterialType::FLEXMESH_STANDARD;
    if (str == "flexmesh_transparent") def.type = ManagedMaterialType::FLEXMESH_TRANSPARENT;
    
    // Textures:
    def.diffuse_map = m_context->getTokString(2);
    if (n > 3) def.specular_map = m_context->getTokString(3);
    if (n > 4) def.damaged_diffuse_map = m_context->getTokString(4);

    m_module->managedmaterials.push_back(def);
}

void AddonPartUtility::ProcessProp()
{
    RigDef::Prop def;
    int n = m_context->countLineArgs();
    if (n < 10)
    {
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Error parsing addonpart file '{}': 'install_prop' has only {} arguments, expected {}",
                m_addonpart_entry->fname, n, 10));
        return;
    }

    int importflags = Node::Ref::REGULAR_STATE_IS_VALID | Node::Ref::REGULAR_STATE_IS_NUMBERED;
    def.reference_node = Node::Ref("", (unsigned int)m_context->getTokFloat(0), importflags, 0);
    def.x_axis_node    = Node::Ref("", (unsigned int)m_context->getTokFloat(1), importflags, 0);
    def.y_axis_node    = Node::Ref("", (unsigned int)m_context->getTokFloat(2), importflags, 0);

    def.offset.x = m_context->getTokFloat(3);
    def.offset.y = m_context->getTokFloat(4);
    def.offset.z = m_context->getTokFloat(5);

    def.rotation.x = m_context->getTokFloat(6);
    def.rotation.y = m_context->getTokFloat(7);
    def.rotation.z = m_context->getTokFloat(8);

    def.mesh_name = m_context->getTokString(9);

    m_module->props.push_back(def);
}

void AddonPartUtility::ProcessFlexbody()
{
    Flexbody def;
    int n = m_context->countLineArgs();
    if (n < 10)
    {
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Error parsing addonpart file '{}': flexbody has only {} arguments, expected {}", m_addonpart_entry->fname, n, 10));
        return;
    }

    int importflags = Node::Ref::REGULAR_STATE_IS_VALID | Node::Ref::REGULAR_STATE_IS_NUMBERED;
    def.reference_node = Node::Ref("", (unsigned int)m_context->getTokFloat(0), importflags, 0);
    def.x_axis_node    = Node::Ref("", (unsigned int)m_context->getTokFloat(1), importflags, 0);
    def.y_axis_node    = Node::Ref("", (unsigned int)m_context->getTokFloat(2), importflags, 0);

    def.offset.x = m_context->getTokFloat(3);
    def.offset.y = m_context->getTokFloat(4);
    def.offset.z = m_context->getTokFloat(5);

    def.rotation.x = m_context->getTokFloat(6);
    def.rotation.y = m_context->getTokFloat(7);
    def.rotation.z = m_context->getTokFloat(8);

    def.mesh_name = m_context->getTokString(9);

    m_context->seekNextLine();

    if (!m_context->isTokString())
    {
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Error parsing addonpart file '{}': flexbody is not followed by 'forset'!", m_addonpart_entry->fname));
        return;
    }

    Parser::ProcessForsetLine(def, m_context->getTokString());

    m_module->flexbodies.push_back(def);
}

