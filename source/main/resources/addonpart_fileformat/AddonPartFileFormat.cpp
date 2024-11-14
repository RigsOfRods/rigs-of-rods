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

#include "Actor.h"
#include "Application.h"
#include "CacheSystem.h"
#include "Console.h"
#include "GameContext.h"
#include "GenericFileFormat.h"
#include "GUI_MessageBox.h"
#include "RigDef_Parser.h"
#include "TuneupFileFormat.h"

#include <Ogre.h>

using namespace RoR;
using namespace RigDef;

AddonPartUtility::AddonPartUtility(bool silent_mode)
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
                        || keyword == Keyword::FLEXBODIES
                        || keyword == Keyword::FLARES
                        || keyword == Keyword::FLARES2)
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
                case Keyword::FLARES: this->ProcessFlare(); break;
                case Keyword::FLARES2: this->ProcessFlare2(); break;
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
                else if (m_context->getTokKeyword() == "addonpart_unwanted_flare" )
                    this->ProcessUnwantedFlare();
                else if (m_context->getTokKeyword() == "addonpart_unwanted_exhaust" )
                    this->ProcessUnwantedExhaust();
                else if (m_context->getTokKeyword() == "addonpart_unwanted_managedmaterial")
                    this->ProcessUnwantedManagedMat();
                else if (m_context->getTokKeyword() == "addonpart_tweak_wheel")
                    this->ProcessTweakWheel();
                else if (m_context->getTokKeyword() == "addonpart_tweak_node")
                    this->ProcessTweakNode();
                else if (m_context->getTokKeyword() == "addonpart_tweak_prop")
                    this->ProcessTweakProp();
                else if (m_context->getTokKeyword() == "addonpart_tweak_flexbody")
                    this->ProcessTweakFlexbody();
                else if (m_context->getTokKeyword() == "addonpart_tweak_managedmaterial")
                    this->ProcessTweakManagedMat();
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
    tuneup->unwanted_flares.clear();
    
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

    def.offset.x = m_context->getTokNumeric(3);
    def.offset.y = m_context->getTokNumeric(4);
    def.offset.z = m_context->getTokNumeric(5);

    def.rotation.x = m_context->getTokNumeric(6);
    def.rotation.y = m_context->getTokNumeric(7);
    def.rotation.z = m_context->getTokNumeric(8);

    def.mesh_name = m_context->getTokString(9);
    def.special = RigDef::Parser::IdentifySpecialProp(def.mesh_name);

    switch (def.special)
    {
    case SpecialProp::BEACON:
        if (n >= 14)
        {
            def.special_prop_beacon.flare_material_name = m_context->getTokString(10);
            Ogre::StringUtil::trim(def.special_prop_beacon.flare_material_name);

            def.special_prop_beacon.color = Ogre::ColourValue(
                m_context->getTokFloat(11), m_context->getTokFloat(12), m_context->getTokFloat(13));
        }
        break;

    case SpecialProp::DASHBOARD_LEFT:
    case SpecialProp::DASHBOARD_RIGHT:
        if (n > 10)
        {
            def.special_prop_dashboard.mesh_name = m_context->getTokString(10);
        }
        if (n > 13)
        {
            def.special_prop_dashboard.offset = Ogre::Vector3(m_context->getTokFloat(11), m_context->getTokFloat(12), m_context->getTokFloat(13));
            def.special_prop_dashboard._offset_is_set = true;
        }
        if (n > 14)
        {
            def.special_prop_dashboard.rotation_angle = m_context->getTokFloat(14);
        }
        break;

    default:
        break;
    }

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
    def.reference_node = Node::Ref("", (unsigned int)m_context->getTokInt(0), importflags, 0);
    def.x_axis_node    = Node::Ref("", (unsigned int)m_context->getTokInt(1), importflags, 0);
    def.y_axis_node    = Node::Ref("", (unsigned int)m_context->getTokInt(2), importflags, 0);

    def.offset.x = m_context->getTokNumeric(3);
    def.offset.y = m_context->getTokNumeric(4);
    def.offset.z = m_context->getTokNumeric(5);

    def.rotation.x = m_context->getTokNumeric(6);
    def.rotation.y = m_context->getTokNumeric(7);
    def.rotation.z = m_context->getTokNumeric(8);

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

void AddonPartUtility::ProcessFlare()
{
    int n = m_context->countLineArgs();
    if (n < 5)
    {
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Error parsing addonpart file '{}': flare has only {} arguments, expected {}", m_addonpart_entry->fname, n, 5));
        return;
    }

    Flare2 def; // We auto-import 'flares' as 'flares2', leaving the `offset.z` at 0.
    int importflags = Node::Ref::REGULAR_STATE_IS_VALID | Node::Ref::REGULAR_STATE_IS_NUMBERED;
    def.reference_node = Node::Ref("", (unsigned int)m_context->getTokInt(0), importflags, 0);
    def.node_axis_x    = Node::Ref("", (unsigned int)m_context->getTokInt(1), importflags, 0);
    def.node_axis_y    = Node::Ref("", (unsigned int)m_context->getTokInt(2), importflags, 0);
    def.offset.x       = m_context->getTokNumeric(3);
    def.offset.y       = m_context->getTokNumeric(4);

    if (n > 5) def.type = (FlareType)m_context->getTokString(5)[0];

    if (n > 6)
    {
        switch (def.type)
        {
            case FlareType::USER:      def.control_number = m_context->getTokInt(6); break;
            case FlareType::DASHBOARD: def.dashboard_link = m_context->getTokString(6); break;
            default: break;
        }
    }

    if (n > 7) { def.blink_delay_milis = m_context->getTokInt(7); }
    if (n > 8) { def.size              = m_context->getTokNumeric(8); }
    if (n > 9) { def.material_name     = m_context->getTokString(9); }

    m_module->flares2.push_back(def);
}

void AddonPartUtility::ProcessFlare2()
{
    int n = m_context->countLineArgs();
    if (n < 6)
    {
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Error parsing addonpart file '{}': flare2 has only {} arguments, expected {}", m_addonpart_entry->fname, n, 6));
        return;
    }

    Flare2 def;
    int importflags = Node::Ref::REGULAR_STATE_IS_VALID | Node::Ref::REGULAR_STATE_IS_NUMBERED;
    def.reference_node = Node::Ref("", (unsigned int)m_context->getTokInt(0), importflags, 0);
    def.node_axis_x    = Node::Ref("", (unsigned int)m_context->getTokInt(1), importflags, 0);
    def.node_axis_y    = Node::Ref("", (unsigned int)m_context->getTokInt(2), importflags, 0);
    def.offset.x       = m_context->getTokNumeric(3);
    def.offset.y       = m_context->getTokNumeric(4);
    def.offset.z       = m_context->getTokNumeric(5); // <-- Specific to 'flares2' (the only difference)

    if (n > 6) def.type = (FlareType)m_context->getTokString(6)[0];

    if (n > 7)
    {
        switch (def.type)
        {
            case FlareType::USER:      def.control_number = m_context->getTokInt(7); break;
            case FlareType::DASHBOARD: def.dashboard_link = m_context->getTokString(7); break;
            default: break;
        }
    }

    if (n > 8) { def.blink_delay_milis = m_context->getTokInt(8); }
    if (n > 9) { def.size              = m_context->getTokNumeric(9); }
    if (n > 10) { def.material_name     = m_context->getTokString(10); }

    m_module->flares2.push_back(def);
}

// Helpers of `ResolveUnwantedAndTweakedElements()`, they expect `m_context` to be in position:

void AddonPartUtility::ProcessUnwantedProp()
{
    ROR_ASSERT(m_context->getTokKeyword() == "addonpart_unwanted_prop"); // also asserts !EOF and TokenType::KEYWORD

    if (m_context->isTokInt(1))
    {
        if (!m_tuneup->isPropProtected((PropID_t)m_context->getTokInt(1)))
        {
            m_tuneup->unwanted_props.insert((PropID_t)m_context->getTokFloat(1));
            this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': marking prop '{}' as UNWANTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), m_context->getTokInt(1)));
        }
        else
        {
            this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': skipping prop '{}' because it's marked PROTECTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), m_context->getTokString(1)));
        }
    }
    else
    {
        this->Log(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': bad arguments", m_addonpart_entry->fname, m_context->getTokKeyword()));
    }
}

void AddonPartUtility::ProcessUnwantedFlexbody()
{
    ROR_ASSERT(m_context->getTokKeyword() == "addonpart_unwanted_flexbody"); // also asserts !EOF and TokenType::KEYWORD

    if (m_context->isTokInt(1))
    {
        if (!m_tuneup->isFlexbodyProtected((FlexbodyID_t)m_context->getTokInt(1)))
        {
            m_tuneup->unwanted_flexbodies.insert((FlexbodyID_t)m_context->getTokFloat(1));
            this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': marking flexbody '{}' as UNWANTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), m_context->getTokInt(1)));
        }
        else
        {
            this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': skipping flexbody '{}' because it's marked PROTECTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), m_context->getTokString(1)));
        }
    }
    else
    {
        this->Log(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': bad arguments", m_addonpart_entry->fname, m_context->getTokKeyword()));
    }
}

void AddonPartUtility::ProcessUnwantedFlare()
{
    ROR_ASSERT(m_context->getTokKeyword() == "addonpart_unwanted_flare"); // also asserts !EOF and TokenType::KEYWORD

    if (m_context->isTokInt(1))
    {
        if (!m_tuneup->isFlareProtected((FlareID_t)m_context->getTokInt(1)))
        {
            m_tuneup->unwanted_flares.insert((FlareID_t)m_context->getTokFloat(1));
            this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': marking flare '{}' as UNWANTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), m_context->getTokInt(1)));
        }
        else
        {
            this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': skipping flare '{}' because it's marked PROTECTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), m_context->getTokString(1)));
        }
    }
    else
    {
        this->Log(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': bad arguments", m_addonpart_entry->fname, m_context->getTokKeyword()));
    }
}

void AddonPartUtility::ProcessUnwantedExhaust()
{
    ROR_ASSERT(m_context->getTokKeyword() == "addonpart_unwanted_exhaust"); // also asserts !EOF and TokenType::KEYWORD

    if (m_context->isTokInt(1))
    {
        if (!m_tuneup->isExhaustProtected((ExhaustID_t)m_context->getTokInt(1)))
        {
            m_tuneup->unwanted_exhausts.insert((ExhaustID_t)m_context->getTokFloat(1));
            this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': marking exhaust '{}' as UNWANTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), m_context->getTokInt(1)));
        }
        else
        {
            this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': skipping exhaust '{}' because it's marked PROTECTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), m_context->getTokString(1)));
        }
    }
    else
    {
        this->Log(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': bad arguments", m_addonpart_entry->fname, m_context->getTokKeyword()));
    }
}

void AddonPartUtility::ProcessUnwantedManagedMat()
{
    ROR_ASSERT(m_context->getTokKeyword() == "addonpart_unwanted_managedmaterial"); // also asserts !EOF and TokenType::KEYWORD

    if (m_context->isTokString(1))
    {
        std::string mat_name = m_context->getTokString(1);
        if (!m_tuneup->isManagedMatProtected(mat_name))
        {
            m_tuneup->unwanted_managedmats.insert(mat_name);
            this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': marking managedmaterial '{}' as UNWANTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), mat_name));
        }
        else
        {
            this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': skipping managedmaterial '{}' because it's marked PROTECTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), mat_name));
        }
    }
    else
    {
        this->Log(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': bad arguments", m_addonpart_entry->fname, m_context->getTokKeyword()));
    }
}

void AddonPartUtility::ProcessTweakWheel()
{
    ROR_ASSERT(m_context->getTokKeyword() == "addonpart_tweak_wheel"); // also asserts !EOF and TokenType::KEYWORD

    if (m_context->isTokInt(1) && m_context->isTokString(2))
    {
        const int wheel_id = m_context->getTokInt(1);
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
            
                this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': Sheduling tweak for wheel '{}'"
                    " with params {{ media1={}, media2={}, side={}, tire_radius={}, rim_radius={} }}",
                    m_addonpart_entry->fname, m_context->getTokKeyword(), wheel_id,
                    data.twt_media[0], data.twt_media[1], (char)data.twt_side, data.twt_tire_radius, data.twt_rim_radius));
            }
            else if (m_tuneup->wheel_tweaks[wheel_id].twt_origin != m_addonpart_entry->fname)
            {
                this->Log(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': Resetting tweaks for wheel '{}' due to conflict with '{}'",
                    m_addonpart_entry->fname, m_context->getTokKeyword(), wheel_id,
                    m_tuneup->wheel_tweaks[wheel_id].twt_origin));

                m_tuneup->wheel_tweaks.erase(wheel_id);
            }
        }
        else
        {
            this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': skipping wheel '{}' because it's marked PROTECTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), m_context->getTokInt(1)));
        }
    }
    else
    {
        this->Log(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': bad arguments", m_addonpart_entry->fname, m_context->getTokKeyword()));
    }
}

void AddonPartUtility::ProcessTweakNode()
{
    ROR_ASSERT(m_context->getTokKeyword() == "addonpart_tweak_node"); // also asserts !EOF and TokenType::KEYWORD

    if (m_context->isTokInt(1) && m_context->isTokNumeric(1) && m_context->isTokNumeric(2) && m_context->isTokNumeric(3))
    {
        NodeNum_t nodenum = (NodeNum_t)m_context->getTokInt(1);
        if (!m_tuneup->isNodeProtected(nodenum))
        {
            if (m_tuneup->node_tweaks.find(nodenum) == m_tuneup->node_tweaks.end())
            {
                TuneupNodeTweak data;
                data.tnt_origin = m_addonpart_entry->fname;
                data.tnt_nodenum = nodenum;
                data.tnt_pos.x = m_context->getTokNumeric(2);
                data.tnt_pos.y = m_context->getTokNumeric(3);
                data.tnt_pos.z = m_context->getTokNumeric(4);
                m_tuneup->node_tweaks.insert(std::make_pair(nodenum, data));
            
                this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': Scheduling tweak for node '{}'"
                    " with params {{ x={}, y={}, z={} }}",
                    m_addonpart_entry->fname, m_context->getTokKeyword(), nodenum,
                    data.tnt_pos.x, data.tnt_pos.y, data.tnt_pos.z));
            }
            else if (m_tuneup->node_tweaks[nodenum].tnt_origin != m_addonpart_entry->fname)
            {
                this->Log(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': Resetting tweaks for node '{}' due to conflict with '{}'",
                    m_addonpart_entry->fname, m_context->getTokKeyword(), nodenum,
                    m_tuneup->node_tweaks[nodenum].tnt_origin));

                m_tuneup->node_tweaks.erase(nodenum);
            }
        }
        else
        {
            this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': skipping node '{}' because it's marked PROTECTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), nodenum));
        }
    }
    else
    {
        this->Log(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': bad arguments", m_addonpart_entry->fname, m_context->getTokKeyword()));
    }
}

void AddonPartUtility::ProcessTweakFlexbody()
{
    ROR_ASSERT(m_context->getTokKeyword() == "addonpart_tweak_flexbody"); // also asserts !EOF and TokenType::KEYWORD

    // TBD: add `null` token type to GenericDocument, so these params can be made optional
    if (m_context->isTokInt(1) && // ID
        m_context->isTokNumeric(2) && m_context->isTokNumeric(3) && m_context->isTokNumeric(4) && // offset
        m_context->isTokNumeric(5) && m_context->isTokNumeric(6) && m_context->isTokNumeric(7) && // rotation
        m_context->isTokString(8)) // media
    {
        const int flexbody_id = m_context->getTokInt(1);
        if (!m_tuneup->isFlexbodyProtected(flexbody_id))
        {
            if (m_tuneup->flexbody_tweaks.find(flexbody_id) == m_tuneup->flexbody_tweaks.end())
            {
                TuneupFlexbodyTweak data;
                data.tft_origin = m_addonpart_entry->fname;
                data.tft_flexbody_id = flexbody_id;
                data.tft_offset.x = m_context->getTokNumeric(2);
                data.tft_offset.y = m_context->getTokNumeric(3);
                data.tft_offset.z = m_context->getTokNumeric(4);
                data.tft_rotation.x = m_context->getTokNumeric(5);
                data.tft_rotation.y = m_context->getTokNumeric(6);
                data.tft_rotation.z = m_context->getTokNumeric(7);
                data.tft_media = m_context->getTokString(8);
                m_tuneup->flexbody_tweaks.insert(std::make_pair(flexbody_id, data));
            
                this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': Scheduling tweak for flexbody '{}'"
                    " with params {{ offsetX={}, offsetY={}, offsetZ={}, rotX={}, rotY={}, rotZ={}, media={} }}",
                    m_addonpart_entry->fname, m_context->getTokKeyword(), flexbody_id, 
                    data.tft_offset.x, data.tft_offset.y, data.tft_offset.z,
                    data.tft_rotation.x, data.tft_rotation.y, data.tft_rotation.z, data.tft_media[0]));
            }
            else if (m_tuneup->flexbody_tweaks[flexbody_id].tft_origin != m_addonpart_entry->fname)
            {
                this->Log(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': Resetting tweaks for flexbody '{}' due to conflict with '{}'",
                    m_addonpart_entry->fname, m_context->getTokKeyword(), flexbody_id,
                    m_tuneup->flexbody_tweaks[flexbody_id].tft_origin));

                m_tuneup->flexbody_tweaks.erase(flexbody_id);
            }
        }
        else
        {
            this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': skipping flexbody '{}' because it's marked PROTECTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), m_context->getTokInt(1)));
        }
    }
    else
    {
        this->Log(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': bad arguments", m_addonpart_entry->fname, m_context->getTokKeyword()));
    }
}

void AddonPartUtility::ProcessTweakProp()
{
    ROR_ASSERT(m_context->getTokKeyword() == "addonpart_tweak_prop"); // also asserts !EOF and TokenType::KEYWORD

    // TBD: add `null` token type to GenericDocument, so these params can be made optional
    if (m_context->isTokInt(1) && // ID
        m_context->isTokNumeric(2) && m_context->isTokNumeric(3) && m_context->isTokNumeric(4) && // offset
        m_context->isTokNumeric(5) && m_context->isTokNumeric(6) && m_context->isTokNumeric(7) && // rotation
        m_context->isTokString(8)) // media
    {
        const int prop_id = m_context->getTokInt(1);
        if (!m_tuneup->isPropProtected(prop_id))
        {
            if (m_tuneup->prop_tweaks.find(prop_id) == m_tuneup->prop_tweaks.end())
            {
                TuneupPropTweak data;
                data.tpt_origin = m_addonpart_entry->fname;
                data.tpt_prop_id = prop_id;
                
                data.tpt_offset.x = m_context->getTokNumeric(2);
                data.tpt_offset.y = m_context->getTokNumeric(3);
                data.tpt_offset.z = m_context->getTokNumeric(4);
                data.tpt_rotation.x = m_context->getTokNumeric(5);
                data.tpt_rotation.y = m_context->getTokNumeric(6);
                data.tpt_rotation.z = m_context->getTokNumeric(7);
                data.tpt_media[0] = m_context->getTokString(8);
                if (m_context->isTokString(9)) data.tpt_media[1] = m_context->getTokString(9); // <== Optional Media2 is specific for prop
                m_tuneup->prop_tweaks.insert(std::make_pair(prop_id, data));
            
                this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': Scheduling tweak for prop '{}'"
                    " with params {{ media1={}, offsetX={}, offsetY={}, offsetZ={}, rotX={}, rotY={}, rotZ={}, media2={} }}",
                    m_addonpart_entry->fname, m_context->getTokKeyword(), prop_id, data.tpt_media[0], 
                    data.tpt_offset.x, data.tpt_offset.y, data.tpt_offset.z,
                    data.tpt_rotation.x, data.tpt_rotation.y, data.tpt_rotation.z,
                    data.tpt_media[1]));
            }
            else if (m_tuneup->prop_tweaks[prop_id].tpt_origin != m_addonpart_entry->fname)
            {
                this->Log(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': Resetting tweaks for prop '{}' due to conflict with '{}'",
                    m_addonpart_entry->fname, m_context->getTokKeyword(), prop_id,
                    m_tuneup->prop_tweaks[prop_id].tpt_origin));

                m_tuneup->prop_tweaks.erase(prop_id);
            }
        }
        else
        {
            this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': skipping prop '{}' because it's marked PROTECTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), m_context->getTokInt(1)));
        }
    }
    else
    {
        this->Log(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': bad arguments", m_addonpart_entry->fname, m_context->getTokKeyword()));
    }
}

void AddonPartUtility::ProcessTweakManagedMat()
{
    ROR_ASSERT(m_context->getTokKeyword() == "addonpart_tweak_managedmaterial"); // also asserts !EOF and TokenType::KEYWORD

    if (m_context->isTokString(1) && m_context->isTokString(2))
    {
        const std::string& mat_name = m_context->getTokString(1);
        if (!m_tuneup->isManagedMatProtected(mat_name))
        {
            if (m_tuneup->managedmat_tweaks.find(mat_name) == m_tuneup->managedmat_tweaks.end())
            {
                TuneupManagedMatTweak data;
                bool stop=false;
                data.tmt_origin = m_addonpart_entry->fname;
                data.tmt_name = mat_name;
                data.tmt_type = m_context->getTokString(2);
                if (!stop && m_context->isTokString(3)) { data.tmt_media[0] = m_context->getTokString(3); } else {stop=true;}
                if (!stop && m_context->isTokString(4)) { data.tmt_media[1] = m_context->getTokString(4); } else {stop=true;}
                if (!stop && m_context->isTokString(5)) { data.tmt_media[2] = m_context->getTokString(5); } else {stop=true;}
                m_tuneup->managedmat_tweaks.insert(std::make_pair(mat_name, data));
            
                this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': Scheduling tweak for managed material '{}'"
                    " with params {{ type={}, media1={}, media2={}, media3={} }}",
                    m_addonpart_entry->fname, m_context->getTokKeyword(), mat_name, data.tmt_type, data.tmt_media[0], data.tmt_media[1], data.tmt_media[2]));
            }
            else if (m_tuneup->managedmat_tweaks[mat_name].tmt_origin != m_addonpart_entry->fname)
            {
                this->Log(fmt::format("[RoR|Addonpart] WARNING: file '{}', directive '{}': Resetting tweaks for managed material '{}' due to conflict with '{}'",
                    m_addonpart_entry->fname, m_context->getTokKeyword(), mat_name,
                    m_tuneup->managedmat_tweaks[mat_name].tmt_origin));

                m_tuneup->managedmat_tweaks.erase(mat_name);
            }
        }
        else
        {
            this->Log(fmt::format("[RoR|Addonpart] INFO: file '{}', directive '{}': skipping managed material '{}' because it's marked PROTECTED",
                m_addonpart_entry->fname, m_context->getTokKeyword(), mat_name));
        }
    }
}

void AddonPartUtility::RecordAddonpartConflicts(CacheEntryPtr addonpart1, CacheEntryPtr addonpart2, AddonPartConflictVec& conflicts)
{
    LOG(fmt::format("[RoR|Addonpart] -- Performing `RecordAddonpartConflicts()` between '{}' and '{}' ~ this involves generating dummy tuneups (hence messages below) --", addonpart1->fname, addonpart2->fname));

    // Make sure both addonparts are loaded and cached.
    App::GetCacheSystem()->LoadResource(addonpart1);
    if (!addonpart1->addonpart_data_only)
    {
        addonpart1->addonpart_data_only = new TuneupDef();
        AddonPartUtility util(/*silent mode:*/true);
        util.ResolveUnwantedAndTweakedElements(addonpart1->addonpart_data_only, addonpart1);
    }

    App::GetCacheSystem()->LoadResource(addonpart2);
    if (!addonpart2->addonpart_data_only)
    {
        addonpart2->addonpart_data_only = new TuneupDef();
        AddonPartUtility util(/*silent mode:*/true);
        util.ResolveUnwantedAndTweakedElements(addonpart2->addonpart_data_only, addonpart2);
    }

    // NODE TWEAKS:
    for (auto& i_pair: addonpart1->addonpart_data_only->node_tweaks)
    {
        NodeNum_t suspect = i_pair.second.tnt_nodenum;
        TuneupNodeTweak* offender = nullptr;
        if (TuneupUtil::isNodeTweaked(addonpart2->addonpart_data_only, suspect, offender))
        {
            conflicts.push_back(AddonPartConflict{addonpart1, addonpart2, "addonpart_tweak_node", (int)suspect});
            LOG(fmt::format("[RoR|Addonpart] Found conflict between '{}' and '{}' - node {} is tweaked by both", addonpart1->fname, addonpart2->fname, (int)suspect));
        }
    }

    // WHEEL TWEAKS:
    for (auto& i_pair: addonpart1->addonpart_data_only->wheel_tweaks)
    {
        WheelID_t suspect = i_pair.second.twt_wheel_id;
        TuneupWheelTweak* offender = nullptr;
        if (TuneupUtil::isWheelTweaked(addonpart2->addonpart_data_only, suspect, offender))
        {
            conflicts.push_back(AddonPartConflict{addonpart1, addonpart2, "addonpart_tweak_wheel", (int)suspect});
            LOG(fmt::format("[RoR|Addonpart] Found conflict between '{}' and '{}' - wheel {} is tweaked by both", addonpart1->fname, addonpart2->fname, (int)suspect));
        }
    }

    // PROP TWEAKS:
    for (auto& i_pair:addonpart1->addonpart_data_only->prop_tweaks)
    {
        PropID_t suspect = i_pair.second.tpt_prop_id;
        TuneupPropTweak* offender = nullptr;
        if (TuneupUtil::isPropTweaked(addonpart2->addonpart_data_only, suspect, offender))
        {
            conflicts.push_back(AddonPartConflict{addonpart1, addonpart2, "addonpart_tweak_prop", (int)suspect});
            LOG(fmt::format("[RoR|Addonpart] Found conflict between '{}' and '{}' - prop {} is tweaked by both", addonpart1->fname, addonpart2->fname, (int)suspect));
        }
    }

    // FLEXBODY TWEAKS:
    for (auto& i_pair: addonpart1->addonpart_data_only->flexbody_tweaks)
    {
        FlexbodyID_t suspect = i_pair.second.tft_flexbody_id;
        TuneupFlexbodyTweak* offender = nullptr;
        if (TuneupUtil::isFlexbodyTweaked(addonpart2->addonpart_data_only, suspect, offender))
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

void AddonPartUtility::Log(const std::string& text)
{
    if (!m_silent_mode)
    {
        LOG(text);
    }
}

bool AddonPartUtility::DoubleCheckForAddonpartConflict(ActorPtr target_actor, CacheEntryPtr addonpart_entry)
{
    // Re-check conflicts (request may come from 'Browse all' button or script).
    // -------------------------------------------------------------------------
            
    AddonPartConflictVec conflicts;
    for (const std::string& use_addonpart: target_actor->getWorkingTuneupDef()->use_addonparts)
    {
        CacheEntryPtr use_entry = App::GetCacheSystem()->FindEntryByFilename(LT_AddonPart, /*partial=*/false, use_addonpart);
        AddonPartUtility::RecordAddonpartConflicts(addonpart_entry, use_entry, conflicts);
    }

    if (conflicts.size() > 0)
    {
        // Messagebox text
        GUI::MessageBoxConfig* dialog = new GUI::MessageBoxConfig;
        dialog->mbc_content_width = 700.f;
        dialog->mbc_title = _LC("Tuning", "Cannot install addon part, conflicts were detected.");
        dialog->mbc_text = fmt::format(_LC("Tuning", "Requested addon part: '{}' (file '{}')."), addonpart_entry->dname, addonpart_entry->fname);
        dialog->mbc_text += "\n";
        dialog->mbc_text += fmt::format(_LC("Tuning", "Total conflicts: {}."), conflicts.size());
        dialog->mbc_text += "\n";
        for (size_t i=0; i < conflicts.size(); i++)
        {
            dialog->mbc_text += "\n";
            dialog->mbc_text += fmt::format(_LC("Tuning", "[{}/{}] '{}' (file '{}') conflicts with '{}' #{}."),
                i+1, conflicts.size(),
                conflicts[i].atc_addonpart2->dname, conflicts[i].atc_addonpart2->fname,
                conflicts[i].atc_keyword, conflicts[i].atc_element_id);
        }

        // Messagebox OK button
        GUI::MessageBoxButton ok_btn;
        ok_btn.mbb_caption = _LC("Tuning", "OK");
        ok_btn.mbb_mq_message = MSG_GUI_HIDE_MESSAGE_BOX_REQUESTED;
        dialog->mbc_buttons.push_back(ok_btn);
        
        // Show the messagebox
        App::GetGameContext()->PushMessage(Message(MSG_GUI_SHOW_MESSAGE_BOX_REQUESTED, (void*)dialog));
    }
    return conflicts.size() > 0;
}
