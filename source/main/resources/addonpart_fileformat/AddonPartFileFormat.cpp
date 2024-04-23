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

AddonPartUtility::AddonPartUtility()
{
    // Inits `RefCountingObjectPtr<>` (CacheEntryPtr, GenericDocumentPtr) - shouldn't be in header.
}

AddonPartUtility::~AddonPartUtility()
{
    // Destroys `RefCountingObjectPtr<>` (CacheEntryPtr, GenericDocumentPtr) - shouldn't be in header.
}

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
            if (m_context->isTokKeyword())
            {
                // (ignore 'addonpart_*' directives)
                if (m_context->getTokKeyword().find("addonpart_") != std::string::npos)
                {
                    m_context->seekNextLine();
                    continue;
                }

                keyword = Parser::IdentifyKeyword(m_context->getTokKeyword());
                if (keyword == Keyword::INVALID && m_context->getTokKeyword() == "set_managedmaterials_options")
                {
                    keyword = Keyword::SET_MANAGEDMATERIALS_OPTIONS; // Workaround, don't ask me why the regex doesn't match this.
                }
                if (keyword != Keyword::INVALID)
                {
                    if (keyword == Keyword::SET_MANAGEDMATERIALS_OPTIONS)
                    {
                        this->ProcessDirectiveSetManagedMaterialsOptions();
                        m_context->seekNextLine();
                        continue;
                    }
                    else if (keyword == Keyword::MANAGEDMATERIALS
                        || keyword == Keyword::PROPS
                        || keyword == Keyword::FLEXBODIES)
                    {
                        block = keyword;
                        m_context->seekNextLine();
                        continue;
                    }

                }
            }
                
            if (block != Keyword::INVALID && !m_context->isTokComment() && !m_context->isTokLineBreak())
            {
                switch (block)
                {
                case Keyword::MANAGEDMATERIALS: this->ProcessManagedMaterial(); break;
                case Keyword::PROPS: this->ProcessProp(); break;
                case Keyword::FLEXBODIES: this->ProcessFlexbody(); break;
                default: break;
                }
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

void AddonPartUtility::ResolveUnwantedAndTweakedElements(TuneupDefPtr& tuneup, CacheEntryPtr& addonpart_entry)
{
    // Evaluates 'addonpart_unwanted_*' directives, respecting 'protected_*' directives in the tuneup.
    // Also handles 'addonpart_tweak_*' directives, resolving possible conflicts among used addonparts.
    // ---------------------------------------------------------------------------------------------

    App::GetCacheSystem()->LoadResource(addonpart_entry);
    m_addonpart_entry = addonpart_entry;
    m_tuneup = tuneup;

    try
    {
        Ogre::DataStreamPtr datastream = Ogre::ResourceGroupManager::getSingleton().openResource(addonpart_entry->fname, addonpart_entry->resource_group);

        m_document = new GenericDocument();
        BitMask_t options = GenericDocument::OPTION_ALLOW_SLASH_COMMENTS | GenericDocument::OPTION_ALLOW_NAKED_STRINGS;
        m_document->loadFromDataStream(datastream, options);
        m_context = new GenericDocContext(m_document);

        while (!m_context->endOfFile())
        {
            if (m_context->isTokKeyword())
            {
                if (m_context->getTokKeyword() == "addonpart_unwanted_prop" )
                    this->ProcessUnwantedProp();
                else if (m_context->getTokKeyword() == "addonpart_unwanted_flexbody" )
                    this->ProcessUnwantedFlexbody();
                else if (m_context->getTokKeyword() == "addonpart_tweak_wheel")
                    this->ProcessTweakWheel();
                else if (m_context->getTokKeyword() == "addonpart_tweak_node")
                    this->ProcessTweakNode();
                else if (m_context->getTokKeyword() == "addonpart_tweak_prop")
                    this->ProcessTweakProp();
                else if (m_context->getTokKeyword() == "addonpart_tweak_flexbody")
                    this->ProcessTweakFlexbody();
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

void AddonPartUtility::ResetUnwantedAndTweakedElements(TuneupDefPtr& tuneup)
{
    ROR_ASSERT(tuneup);

    // Unwanted
    tuneup->unwanted_flexbodies.clear();
    tuneup->unwanted_props.clear();
    
    // Tweaked
    tuneup->node_tweaks.clear();
    tuneup->wheel_tweaks.clear();
    tuneup->prop_tweaks.clear();
    tuneup->flexbody_tweaks.clear();
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
    // (placeholders)
    if (def.specular_map == "-") def.specular_map = "";
    if (def.damaged_diffuse_map == "-") def.damaged_diffuse_map = "";

    // Options:
    def.options = m_managedmaterials_options;

    m_module->managedmaterials.push_back(def);
}

void AddonPartUtility::ProcessDirectiveSetManagedMaterialsOptions()
{
    int n = m_context->countLineArgs();
    if (n > 1)
    {
        m_managedmaterials_options.double_sided = m_context->getFloatData(1) == 1.f;
    }
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
    def._mesh_rg_override = m_addonpart_entry->resource_group;

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
    def._mesh_rg_override = m_addonpart_entry->resource_group;

    m_context->seekNextLine();

    if (!m_context->isTokString())
    {
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Error parsing addonpart file '{}': flexbody is not followed by 'forset'!", m_addonpart_entry->fname));
        return;
    }

    Parser::ProcessForsetLine(def, m_context->getTokString());

    // Resolve `forset` ranges:
    for (RigDef::Node::Range const& range: def.node_list_to_import)
    {
        for (unsigned int i = range.start.Num(); i <= range.end.Num(); ++i)
        {
            Node::Ref ref("", i, Node::Ref::REGULAR_STATE_IS_VALID | Node::Ref::REGULAR_STATE_IS_NUMBERED, 0);
            def.node_list.push_back(ref);
        }
    }

    m_module->flexbodies.push_back(def);
}

// Helpers of `ResolveUnwantedAndTweakedElements()`, they expect `m_context` to be in position:

void AddonPartUtility::ProcessUnwantedProp()
{
    ROR_ASSERT(m_context->getTokKeyword() == "addonpart_unwanted_prop"); // also asserts !EOF and TokenType::KEYWORD

    if (m_context->isTokFloat(1))
    {
        if (!m_tuneup->isPropProtected((PropID_t)m_context->getTokFloat(1)))
        {
            m_tuneup->unwanted_props.insert((PropID_t)m_context->getTokFloat(1));
            LOG(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': marking prop '{}' as UNWANTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), (int)m_context->getTokFloat(1)));
        }
        else
        {
            LOG(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': skipping prop '{}' because it's marked PROTECTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), m_context->getTokString(1)));
        }
    }
    else
    {
        LOG(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': bad arguments", m_addonpart_entry->fname, m_context->getTokKeyword()));
    }
}

void AddonPartUtility::ProcessUnwantedFlexbody()
{
    ROR_ASSERT(m_context->getTokKeyword() == "addonpart_unwanted_flexbody"); // also asserts !EOF and TokenType::KEYWORD

    if (m_context->isTokFloat(1))
    {
        if (!m_tuneup->isFlexbodyProtected((FlexbodyID_t)m_context->getTokFloat(1)))
        {
            m_tuneup->unwanted_flexbodies.insert((FlexbodyID_t)m_context->getTokFloat(1));
            LOG(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': marking flexbody '{}' as UNWANTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), (int)m_context->getTokFloat(1)));
        }
        else
        {
            LOG(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': skipping flexbody '{}' because it's marked PROTECTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), m_context->getTokString(1)));
        }
    }
    else
    {
        LOG(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': bad arguments", m_addonpart_entry->fname, m_context->getTokKeyword()));
    }
}

void AddonPartUtility::ProcessTweakWheel()
{
    ROR_ASSERT(m_context->getTokKeyword() == "addonpart_tweak_wheel"); // also asserts !EOF and TokenType::KEYWORD

    if (m_context->isTokFloat(1) && m_context->isTokString(2))
    {
        const int wheel_id = (int)m_context->getTokFloat(1);
        if (!m_tuneup->isWheelProtected(wheel_id))
        {
            if (m_tuneup->wheel_tweaks.find(wheel_id) == m_tuneup->wheel_tweaks.end())
            {
                TuneupWheelTweak data;
                bool stop = false;
                data.twt_origin = m_addonpart_entry->fname;
                data.twt_wheel_id = wheel_id;
                data.twt_media[0] = m_context->getTokString(2);
                if (!stop && m_context->isTokString(3)) { data.twt_media[1] = m_context->getTokString(3); } else { stop=true; }
                if (!stop && m_context->isTokString(4)) { data.twt_side = (m_context->getTokString(4)[0] == 'l') ? WheelSide::LEFT : WheelSide::RIGHT; } else { stop=true; }
                if (!stop && m_context->isTokFloat(5)) { data.twt_tire_radius = m_context->getTokFloat(5); } else { stop=true; }
                if (!stop && m_context->isTokFloat(6)) { data.twt_rim_radius = m_context->getTokFloat(6); } else { stop=true; }
                m_tuneup->wheel_tweaks.insert(std::make_pair(wheel_id, data));
            
                LOG(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': Sheduling tweak for wheel '{}'"
                    " with params {{ media1={}, media2={}, side={}, tire_radius={}, rim_radius={} }}",
                    m_addonpart_entry->fname, m_context->getTokKeyword(), wheel_id,
                    data.twt_media[0], data.twt_media[1], (char)data.twt_side, data.twt_tire_radius, data.twt_rim_radius));
            }
            else if (m_tuneup->wheel_tweaks[wheel_id].twt_origin != m_addonpart_entry->fname)
            {
                LOG(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': Resetting tweaks for wheel '{}' due to conflict with '{}'",
                    m_addonpart_entry->fname, m_context->getTokKeyword(), wheel_id,
                    m_tuneup->wheel_tweaks[wheel_id].twt_origin));

                m_tuneup->wheel_tweaks.erase(wheel_id);
            }
        }
        else
        {
            LOG(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': skipping wheel '{}' because it's marked PROTECTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), (int)m_context->getTokFloat(1)));
        }
    }
    else
    {
        LOG(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': bad arguments", m_addonpart_entry->fname, m_context->getTokKeyword()));
    }
}

void AddonPartUtility::ProcessTweakNode()
{
    ROR_ASSERT(m_context->getTokKeyword() == "addonpart_tweak_node"); // also asserts !EOF and TokenType::KEYWORD

    if (m_context->isTokFloat(1) && m_context->isTokFloat(1) && m_context->isTokFloat(2) && m_context->isTokFloat(3))
    {
        NodeNum_t nodenum = (NodeNum_t)m_context->getTokFloat(1);
        if (!m_tuneup->isNodeProtected(nodenum))
        {
            if (m_tuneup->node_tweaks.find(nodenum) == m_tuneup->node_tweaks.end())
            {
                TuneupNodeTweak data;
                data.tnt_origin = m_addonpart_entry->fname;
                data.tnt_nodenum = nodenum;
                data.tnt_pos.x = m_context->getTokFloat(2);
                data.tnt_pos.y = m_context->getTokFloat(3);
                data.tnt_pos.z = m_context->getTokFloat(4);
                m_tuneup->node_tweaks.insert(std::make_pair(nodenum, data));
            
                LOG(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': Scheduling tweak for node '{}'"
                    " with params {{ x={}, y={}, z={} }}",
                    m_addonpart_entry->fname, m_context->getTokKeyword(), nodenum,
                    data.tnt_pos.x, data.tnt_pos.y, data.tnt_pos.z));
            }
            else if (m_tuneup->node_tweaks[nodenum].tnt_origin != m_addonpart_entry->fname)
            {
                LOG(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': Resetting tweaks for node '{}' due to conflict with '{}'",
                    m_addonpart_entry->fname, m_context->getTokKeyword(), nodenum,
                    m_tuneup->node_tweaks[nodenum].tnt_origin));

                m_tuneup->node_tweaks.erase(nodenum);
            }
        }
        else
        {
            LOG(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': skipping node '{}' because it's marked PROTECTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), nodenum));
        }
    }
    else
    {
        LOG(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': bad arguments", m_addonpart_entry->fname, m_context->getTokKeyword()));
    }
}

void AddonPartUtility::ProcessTweakFlexbody()
{
    ROR_ASSERT(m_context->getTokKeyword() == "addonpart_tweak_flexbody"); // also asserts !EOF and TokenType::KEYWORD

    // TBD: add `null` token type to GenericDocument, so these params can be made optional
    if (m_context->isTokFloat(1) && // ID
        m_context->isTokFloat(2) && m_context->isTokFloat(3) && m_context->isTokFloat(4) && // offset
        m_context->isTokFloat(5) && m_context->isTokFloat(6) && m_context->isTokFloat(7) && // rotation
        m_context->isTokString(8)) // media
    {
        const int flexbody_id = (int)m_context->getTokFloat(1);
        if (!m_tuneup->isFlexbodyProtected(flexbody_id))
        {
            if (m_tuneup->flexbody_tweaks.find(flexbody_id) == m_tuneup->flexbody_tweaks.end())
            {
                TuneupFlexbodyTweak data;
                data.tft_origin = m_addonpart_entry->fname;
                data.tft_flexbody_id = flexbody_id;
                data.tft_offset.x = m_context->getTokFloat(2);
                data.tft_offset.y = m_context->getTokFloat(3);
                data.tft_offset.z = m_context->getTokFloat(4);
                data.tft_rotation.x = m_context->getTokFloat(5);
                data.tft_rotation.y = m_context->getTokFloat(6);
                data.tft_rotation.z = m_context->getTokFloat(7);
                data.tft_media = m_context->getTokString(8);
                m_tuneup->flexbody_tweaks.insert(std::make_pair(flexbody_id, data));
            
                LOG(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': Scheduling tweak for flexbody '{}'"
                    " with params {{ offsetX={}, offsetY={}, offsetZ={}, rotX={}, rotY={}, rotZ={}, media={} }}",
                    m_addonpart_entry->fname, m_context->getTokKeyword(), flexbody_id, 
                    data.tft_offset.x, data.tft_offset.y, data.tft_offset.z,
                    data.tft_rotation.x, data.tft_rotation.y, data.tft_rotation.z, data.tft_media[0]));
            }
            else if (m_tuneup->flexbody_tweaks[flexbody_id].tft_origin != m_addonpart_entry->fname)
            {
                LOG(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': Resetting tweaks for flexbody '{}' due to conflict with '{}'",
                    m_addonpart_entry->fname, m_context->getTokKeyword(), flexbody_id,
                    m_tuneup->flexbody_tweaks[flexbody_id].tft_origin));

                m_tuneup->flexbody_tweaks.erase(flexbody_id);
            }
        }
        else
        {
            LOG(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': skipping flexbody '{}' because it's marked PROTECTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), (int)m_context->getTokFloat(1)));
        }
    }
    else
    {
        LOG(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': bad arguments", m_addonpart_entry->fname, m_context->getTokKeyword()));
    }
}

void AddonPartUtility::ProcessTweakProp()
{
    ROR_ASSERT(m_context->getTokKeyword() == "addonpart_tweak_prop"); // also asserts !EOF and TokenType::KEYWORD

    // TBD: add `null` token type to GenericDocument, so these params can be made optional
    if (m_context->isTokFloat(1) && // ID
        m_context->isTokFloat(2) && m_context->isTokFloat(3) && m_context->isTokFloat(4) && // offset
        m_context->isTokFloat(5) && m_context->isTokFloat(6) && m_context->isTokFloat(7) && // rotation
        m_context->isTokString(8)) // media
    {
        const int prop_id = (int)m_context->getTokFloat(1);
        if (!m_tuneup->isPropProtected(prop_id))
        {
            if (m_tuneup->prop_tweaks.find(prop_id) == m_tuneup->prop_tweaks.end())
            {
                TuneupPropTweak data;
                data.tpt_origin = m_addonpart_entry->fname;
                data.tpt_prop_id = prop_id;
                
                data.tpt_offset.x = m_context->getTokFloat(2);
                data.tpt_offset.y = m_context->getTokFloat(3);
                data.tpt_offset.z = m_context->getTokFloat(4);
                data.tpt_rotation.x = m_context->getTokFloat(5);
                data.tpt_rotation.y = m_context->getTokFloat(6);
                data.tpt_rotation.z = m_context->getTokFloat(7);
                data.tpt_media[0] = m_context->getTokString(8);
                if (m_context->isTokString(9)) data.tpt_media[1] = m_context->getTokString(9); // <== Optional Media2 is specific for prop
                m_tuneup->prop_tweaks.insert(std::make_pair(prop_id, data));
            
                LOG(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': Scheduling tweak for prop '{}'"
                    " with params {{ media1={}, offsetX={}, offsetY={}, offsetZ={}, rotX={}, rotY={}, rotZ={}, media2={} }}",
                    m_addonpart_entry->fname, m_context->getTokKeyword(), prop_id, data.tpt_media[0], 
                    data.tpt_offset.x, data.tpt_offset.y, data.tpt_offset.z,
                    data.tpt_rotation.x, data.tpt_rotation.y, data.tpt_rotation.z,
                    data.tpt_media[1]));
            }
            else if (m_tuneup->prop_tweaks[prop_id].tpt_origin != m_addonpart_entry->fname)
            {
                LOG(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': Resetting tweaks for prop '{}' due to conflict with '{}'",
                    m_addonpart_entry->fname, m_context->getTokKeyword(), prop_id,
                    m_tuneup->prop_tweaks[prop_id].tpt_origin));

                m_tuneup->prop_tweaks.erase(prop_id);
            }
        }
        else
        {
            LOG(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': skipping prop '{}' because it's marked PROTECTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), (int)m_context->getTokFloat(1)));
        }
    }
    else
    {
        LOG(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': bad arguments", m_addonpart_entry->fname, m_context->getTokKeyword()));
    }
}

void AddonPartUtility::RecordAddonpartConflicts(CacheEntryPtr addonpart1, CacheEntryPtr addonpart2, AddonPartConflictVec& conflicts)
{
    LOG(fmt::format("[RoR|Addonpart] -- Performing `RecordAddonpartConflicts()` between '{}' and '{}' ~ this involves generating dummy tuneups (hence messages below) --", addonpart1->fname, addonpart2->fname));

    // Load both addonparts to dummy Tuneup instances    
    TuneupDefPtr dummy_t1 = new TuneupDef();
    App::GetCacheSystem()->LoadResource(addonpart1);
    AddonPartUtility util_t1;
    util_t1.ResolveUnwantedAndTweakedElements(dummy_t1, addonpart1);

    TuneupDefPtr dummy_t2 = new TuneupDef();
    App::GetCacheSystem()->LoadResource(addonpart2);
    AddonPartUtility util_t2;
    util_t2.ResolveUnwantedAndTweakedElements(dummy_t2, addonpart2);

    // NODE TWEAKS:
    for (size_t i = 0; i < dummy_t1->node_tweaks.size(); i++)
    {
        NodeNum_t suspect = dummy_t1->node_tweaks[i].tnt_nodenum;
        TuneupNodeTweak* offender = nullptr;
        if (TuneupUtil::isNodeTweaked(dummy_t2, suspect, offender))
        {
            conflicts.push_back(AddonPartConflict{addonpart1, addonpart2, "addonpart_tweak_node", (int)suspect});
            LOG(fmt::format("[RoR|Addonpart] Found conflict between '{}' and '{}' - node {} is tweaked by both", addonpart1->fname, addonpart2->fname, (int)suspect));
        }
    }

    // WHEEL TWEAKS:
    for (size_t i = 0; i < dummy_t1->wheel_tweaks.size(); i++)
    {
        WheelID_t suspect = dummy_t1->wheel_tweaks[i].twt_wheel_id;
        TuneupWheelTweak* offender = nullptr;
        if (TuneupUtil::isWheelTweaked(dummy_t2, suspect, offender))
        {
            conflicts.push_back(AddonPartConflict{addonpart1, addonpart2, "addonpart_tweak_wheel", (int)suspect});
            LOG(fmt::format("[RoR|Addonpart] Found conflict between '{}' and '{}' - wheel {} is tweaked by both", addonpart1->fname, addonpart2->fname, (int)suspect));
        }
    }

    // PROP TWEAKS:
    for (size_t i = 0; i < dummy_t1->prop_tweaks.size(); i++)
    {
        PropID_t suspect = dummy_t1->prop_tweaks[i].tpt_prop_id;
        TuneupPropTweak* offender = nullptr;
        if (TuneupUtil::isPropTweaked(dummy_t2, suspect, offender))
        {
            conflicts.push_back(AddonPartConflict{addonpart1, addonpart2, "addonpart_tweak_prop", (int)suspect});
            LOG(fmt::format("[RoR|Addonpart] Found conflict between '{}' and '{}' - prop {} is tweaked by both", addonpart1->fname, addonpart2->fname, (int)suspect));
        }
    }

    // FLEXBODY TWEAKS:
    for (size_t i = 0; i < dummy_t1->flexbody_tweaks.size(); i++)
    {
        FlexbodyID_t suspect = dummy_t1->flexbody_tweaks[i].tft_flexbody_id;
        TuneupFlexbodyTweak* offender = nullptr;
        if (TuneupUtil::isFlexbodyTweaked(dummy_t2, suspect, offender))
        {
            conflicts.push_back(AddonPartConflict{addonpart1, addonpart2, "addonpart_tweak_flexbody", (int)suspect});
            LOG(fmt::format("[RoR|Addonpart] Found conflict between '{}' and '{}' - flexbody {} is tweaked by both", addonpart1->fname, addonpart2->fname, (int)suspect));
        }
    }

    LOG(fmt::format("[RoR|Addonpart] -- Done with `RecordAddonpartConflicts()` between '{}' and '{}' --", addonpart1->fname, addonpart2->fname));
}

bool AddonPartUtility::CheckForAddonpartConflict(CacheEntryPtr addonpart1, CacheEntryPtr addonpart2, AddonPartConflictVec& conflicts)
{
    if (!addonpart1 || !addonpart2)
    {
        return false;
    }

    for (AddonPartConflict& conflict: conflicts)
    {
        if ((conflict.atc_addonpart1 == addonpart1 && conflict.atc_addonpart2 == addonpart2) ||
            (conflict.atc_addonpart1 == addonpart2 && conflict.atc_addonpart2 == addonpart1))
        {
            return true;
        }
    }
    return false;
}
