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

#ifdef USE_OPENAL

#include "SoundScriptManager.h"

#include "Beam.h"
#include "Sound.h"
#include "SoundManager.h"
#include "Utils.h"

#include <OgreResourceGroupManager.h>

using namespace Ogre;
using namespace RoR;

const float SoundScriptInstance::PITCHDOWN_FADE_FACTOR = 3.0f;
const float SoundScriptInstance::PITCHDOWN_CUTOFF_FACTOR = 5.0f;

SoundScriptManager::SoundScriptManager() :
    disabled(true)
    , loading_base(false)
    , instance_counter(0)
    , max_distance(500.0f)
    , rolloff_factor(1.0f)
    , reference_distance(7.5f)
    , sound_manager(nullptr)
{
    for (int i = 0; i < SS_MAX_TRIG; i++)
    {
        free_trigs[i] = 0;
    }

    for (int i = 0; i < SS_MAX_MOD; i++)
    {
        free_pitches[i] = 0;
        free_gains[i] = 0;
    }

    // TODO: there is a memory corruption going on here, need to fix
    for (int i = 0; i < SS_MAX_TRIG * MAX_INSTANCES_PER_GROUP; i++)
    {
        trigs[i] = 0;
    }

    for (int i = 0; i < SS_MAX_MOD * MAX_INSTANCES_PER_GROUP; i++)
    {
        pitches[i] = 0;
        gains[i] = 0;
    }

    // reset all states
    state_map.clear();

    sound_manager = new SoundManager();

    if (!sound_manager)
    {
        LOG("SoundScriptManager: Failed to create the Sound Manager");
        return;
    }

    disabled = sound_manager->isDisabled();

    if (disabled)
    {
        LOG("SoundScriptManager: Sound Manager is disabled");
        return;
    }

    LOG("SoundScriptManager: Sound Manager started with " + TOSTRING(sound_manager->getNumHardwareSources())+" sources");
    script_patterns.push_back("*.soundscript");
    ResourceGroupManager::getSingleton()._registerScriptLoader(this);
}

SoundScriptManager::~SoundScriptManager()
{
    if (sound_manager != nullptr)
        delete sound_manager;
}

void SoundScriptManager::trigOnce(Actor* actor, int trig, int linkType, int linkItemID)
{
    if (disabled)
        return;

    if (actor)
    {
        trigOnce(actor->ar_instance_id, trig, linkType, linkItemID);
    }
}

void SoundScriptManager::trigOnce(int actor_id, int trig, int linkType, int linkItemID)
{
    if (disabled)
        return;

    for (int i = 0; i < free_trigs[trig]; i++)
    {
        // cycle through all instance groups
        SoundScriptInstance* inst = trigs[trig + i * SS_MAX_TRIG];

        if (inst && inst->actor_id == actor_id && inst->sound_link_type == linkType && inst->sound_link_item_id == linkItemID)
        {
            inst->runOnce();
        }
    }
}

void SoundScriptManager::trigStart(Actor* actor, int trig, int linkType, int linkItemID)
{
    if (disabled)
        return;

    if (actor)
    {
        trigStart(actor->ar_instance_id, trig, linkType, linkItemID);
    }
}

void SoundScriptManager::trigStart(int actor_id, int trig, int linkType, int linkItemID)
{
    if (disabled)
        return;
    if (getTrigState(actor_id, trig, linkType, linkItemID))
        return;

    state_map[linkType][linkItemID][actor_id][trig] = true;

    for (int i = 0; i < free_trigs[trig]; i++)
    {
        SoundScriptInstance* inst = trigs[trig + i * SS_MAX_TRIG];

        if (inst && inst->actor_id == actor_id && inst->sound_link_type == linkType && inst->sound_link_item_id == linkItemID)
        {
            inst->start();
        }
    }
}

void SoundScriptManager::trigStop(Actor* actor, int trig, int linkType, int linkItemID)
{
    if (disabled)
        return;

    if (actor)
    {
        trigStop(actor->ar_instance_id, trig, linkType, linkItemID);
    }
}

void SoundScriptManager::trigStop(int actor_id, int trig, int linkType, int linkItemID)
{
    if (disabled)
        return;
    if (!getTrigState(actor_id, trig, linkType, linkItemID))
        return;

    state_map[linkType][linkItemID][actor_id][trig] = false;
    for (int i = 0; i < free_trigs[trig]; i++)
    {
        SoundScriptInstance* inst = trigs[trig + i * SS_MAX_TRIG];

        if (inst && inst->actor_id == actor_id && inst->sound_link_type == linkType && inst->sound_link_item_id == linkItemID)
        {
            inst->stop();
        }
    }
}

void SoundScriptManager::trigKill(Actor* actor, int trig, int linkType, int linkItemID)
{
    if (disabled)
        return;

    if (actor)
    {
        trigKill(actor->ar_instance_id, trig, linkType, linkItemID);
    }
}

void SoundScriptManager::trigKill(int actor_id, int trig, int linkType, int linkItemID)
{
    if (disabled)
        return;
    if (!getTrigState(actor_id, trig, linkType, linkItemID))
        return;

    state_map[linkType][linkItemID][actor_id][trig] = false;
    for (int i = 0; i < free_trigs[trig]; i++)
    {
        SoundScriptInstance* inst = trigs[trig + i * SS_MAX_TRIG];

        if (inst && inst->actor_id == actor_id && inst->sound_link_type == linkType && inst->sound_link_item_id == linkItemID)
        {
            inst->kill();
        }
    }
}

void SoundScriptManager::trigToggle(Actor* actor, int trig, int linkType, int linkItemID)
{
    if (disabled)
        return;

    if (actor)
    {
        trigToggle(actor->ar_instance_id, trig, linkType, linkItemID);
    }
}

void SoundScriptManager::trigToggle(int actor_id, int trig, int linkType, int linkItemID)
{
    if (disabled)
        return;

    if (getTrigState(actor_id, trig, linkType, linkItemID))
        trigStop(actor_id, trig, linkType, linkItemID);
    else
        trigStart(actor_id, trig, linkType, linkItemID);
}

bool SoundScriptManager::getTrigState(Actor* actor, int trig, int linkType, int linkItemID)
{
    if (disabled)
        return false;

    if (actor)
        return getTrigState(actor->ar_instance_id, trig, linkType, linkItemID);
    else
        return false;
}

bool SoundScriptManager::getTrigState(int actor_id, int trig, int linkType, int linkItemID)
{
    if (disabled)
        return false;

    return state_map[linkType][linkItemID][actor_id][trig];
}

void SoundScriptManager::modulate(Actor* actor, int mod, float value, int linkType, int linkItemID)
{
    if (disabled)
        return;

    if (actor)
    {
        modulate(actor->ar_instance_id, mod, value, linkType, linkItemID);
    }
}

void SoundScriptManager::modulate(int actor_id, int mod, float value, int linkType, int linkItemID)
{
    if (disabled)
        return;

    if (mod >= SS_MAX_MOD)
        return;

    for (int i = 0; i < free_gains[mod]; i++)
    {
        SoundScriptInstance* inst = gains[mod + i * SS_MAX_MOD];
        if (inst && inst->actor_id == actor_id && inst->sound_link_type == linkType && inst->sound_link_item_id == linkItemID)
        {
            // this one requires modulation
            float gain = value * value * inst->templ->gain_square + value * inst->templ->gain_multiplier + inst->templ->gain_offset;
            gain = std::max(0.0f, gain);
            gain = std::min(gain, 1.0f);
            inst->setGain(gain);
        }
    }

    for (int i = 0; i < free_pitches[mod]; i++)
    {
        SoundScriptInstance* inst = pitches[mod + i * SS_MAX_MOD];
        if (inst && inst->actor_id == actor_id && inst->sound_link_type == linkType && inst->sound_link_item_id == linkItemID)
        {
            // this one requires modulation
            float pitch = value * value * inst->templ->pitch_square + value * inst->templ->pitch_multiplier + inst->templ->pitch_offset;
            pitch = std::max(0.0f, pitch);
            inst->setPitch(pitch);
        }
    }
}

void SoundScriptManager::setCamera(Vector3 position, Vector3 direction, Vector3 up, Vector3 velocity)
{
    if (disabled)
        return;
    sound_manager->setCamera(position, direction, up, velocity);
}

const StringVector& SoundScriptManager::getScriptPatterns(void) const
{
    return script_patterns;
}

Real SoundScriptManager::getLoadingOrder(void) const
{
    // load late
    return 1000.0f;
}

SoundScriptTemplate* SoundScriptManager::createTemplate(String name, String groupname, String filename)
{
    // first, search if there is a template name collision
    if (templates.find(name) != templates.end())
    {
        LOG("SoundScriptManager::createTemplate(): SoundScript with name [" + name + "] already exists, skipping...");
        return nullptr;
    }

    SoundScriptTemplate* ssi = new SoundScriptTemplate(name, groupname, filename, loading_base);
    templates[name] = ssi;
    return ssi;
}

SoundScriptInstance* SoundScriptManager::createInstance(Ogre::String templatename, int actor_id, Ogre::SceneNode* toAttach, int soundLinkType, int soundLinkItemId)
{
    //first, search template
    SoundScriptTemplate* templ = NULL;

    if (templates.find(templatename) == templates.end())
    {
        return NULL; // found no template with this name
    }

    templ = templates[templatename];

    if (templ->trigger_source == SS_TRIG_NONE)
    {
        return NULL; // invalid template!
    }

    if (free_trigs[templ->trigger_source] >= MAX_INSTANCES_PER_GROUP
        || (free_gains[templ->trigger_source] >= MAX_INSTANCES_PER_GROUP && templ->gain_source != SS_MOD_NONE)
        || (free_pitches[templ->trigger_source] >= MAX_INSTANCES_PER_GROUP && templ->pitch_source != SS_MOD_NONE))
    {
        LOG("SoundScriptManager: Reached MAX_INSTANCES_PER_GROUP limit (" + TOSTRING(MAX_INSTANCES_PER_GROUP) + ")");
        return NULL; // reached limit!
    }

    SoundScriptInstance* inst = new SoundScriptInstance(actor_id, templ, sound_manager, templ->file_name + "-" + TOSTRING(actor_id) + "-" + TOSTRING(instance_counter), soundLinkType, soundLinkItemId);
    instance_counter++;

    // register to lookup tables
    trigs[templ->trigger_source + free_trigs[templ->trigger_source] * SS_MAX_TRIG] = inst;
    free_trigs[templ->trigger_source]++;

    if (templ->gain_source != SS_MOD_NONE)
    {
        gains[templ->gain_source + free_gains[templ->gain_source] * SS_MAX_MOD] = inst;
        free_gains[templ->gain_source]++;
    }
    if (templ->pitch_source != SS_MOD_NONE)
    {
        pitches[templ->pitch_source + free_pitches[templ->pitch_source] * SS_MAX_MOD] = inst;
        free_pitches[templ->pitch_source]++;
    }

    // SoundTrigger: SS_TRIG_ALWAYSON
    if (templ->trigger_source == SS_TRIG_ALWAYSON)
    {
        inst->start();
    }

    return inst;
}

void SoundScriptManager::parseScript(DataStreamPtr& stream, const String& groupName)
{
    SoundScriptTemplate* sst = 0;
    String line = "";
    std::vector<String> vecparams;

    LOG("SoundScriptManager: Parsing script "+stream->getName());

    while (!stream->eof())
    {
        line = RoR::Utils::SanitizeUtf8String(stream->getLine());
        // ignore comments & blanks
        if (!(line.length() == 0 || line.substr(0, 2) == "//"))
        {
            if (sst == 0)
            {
                // no current SoundScript
                // so first valid data should be a SoundScript name
                LOG("SoundScriptManager: creating template "+line);
                sst = createTemplate(line, groupName, stream->getName());
                if (!sst)
                {
                    // there is a name collision for this Sound Script
                    LOG("SoundScriptManager: Error, this sound script is already defined: "+line);
                    skipToNextOpenBrace(stream);
                    skipToNextCloseBrace(stream);
                    continue;
                }
                // skip to and over next {
                skipToNextOpenBrace(stream);
            }
            else
            {
                // already in a ss
                if (line == "}")
                {
                    // finished ss
                    sst = 0;
                }
                else
                {
                    // attribute
                    // split params on space
                    Ogre::StringVector veclineparams = StringUtil::split(line, "\t ", 0);

                    if (!sst->setParameter(veclineparams))
                    {
                        LOG("Bad SoundScript attribute line: '" + line + "' in " + stream->getName());
                    }
                }
            }
        }
    }
}

void SoundScriptManager::skipToNextCloseBrace(DataStreamPtr& stream)
{
    String line = "";

    while (!stream->eof() && line != "}")
    {
        line = stream->getLine();
    }
}

void SoundScriptManager::skipToNextOpenBrace(DataStreamPtr& stream)
{
    String line = "";

    while (!stream->eof() && line != "{")
    {
        line = stream->getLine();
    }
}

void SoundScriptManager::setEnabled(bool state)
{
    if (state)
        sound_manager->resumeAllSounds();
    else
        sound_manager->pauseAllSounds();
}

//=====================================================================

SoundScriptTemplate::SoundScriptTemplate(String name, String groupname, String filename, bool baseTemplate) :
    base_template(baseTemplate)
    , file_name(filename)
    , free_sound(0)
    , gain_multiplier(1.0f)
    , gain_offset(0.0f)
    , gain_source(SS_MOD_NONE)
    , gain_square(0.0f)
    , has_start_sound(false)
    , has_stop_sound(false)
    , name(name)
    , pitch_multiplier(1.0f)
    , pitch_offset(0.0f)
    , pitch_source(SS_MOD_NONE)
    , pitch_square(0.0f)
    , start_sound_pitch(0.0f)
    , stop_sound_pitch(0.0f)
    , trigger_source(SS_TRIG_NONE)
    , unpitchable(false)
{
}

bool SoundScriptTemplate::setParameter(Ogre::StringVector vec)
{
    if (vec.empty())
        return false;

    if (vec[0] == String("trigger_source"))
    {
        if (vec.size() < 2)
            return false;
        if (vec[1] == String("engine"))
        {
            trigger_source = SS_TRIG_ENGINE;
            return true;
        };
        if (vec[1] == String("aeroengine1"))
        {
            trigger_source = SS_TRIG_AEROENGINE1;
            return true;
        };
        if (vec[1] == String("aeroengine2"))
        {
            trigger_source = SS_TRIG_AEROENGINE2;
            return true;
        };
        if (vec[1] == String("aeroengine3"))
        {
            trigger_source = SS_TRIG_AEROENGINE3;
            return true;
        };
        if (vec[1] == String("aeroengine4"))
        {
            trigger_source = SS_TRIG_AEROENGINE4;
            return true;
        };
        if (vec[1] == String("aeroengine5"))
        {
            trigger_source = SS_TRIG_AEROENGINE5;
            return true;
        };
        if (vec[1] == String("aeroengine6"))
        {
            trigger_source = SS_TRIG_AEROENGINE6;
            return true;
        };
        if (vec[1] == String("aeroengine7"))
        {
            trigger_source = SS_TRIG_AEROENGINE7;
            return true;
        };
        if (vec[1] == String("aeroengine8"))
        {
            trigger_source = SS_TRIG_AEROENGINE8;
            return true;
        };
        if (vec[1] == String("horn"))
        {
            trigger_source = SS_TRIG_HORN;
            return true;
        };
        if (vec[1] == String("brake"))
        {
            trigger_source = SS_TRIG_BRAKE;
            return true;
        };
        if (vec[1] == String("pump"))
        {
            trigger_source = SS_TRIG_PUMP;
            return true;
        };
        if (vec[1] == String("starter"))
        {
            trigger_source = SS_TRIG_STARTER;
            return true;
        };
        if (vec[1] == String("turbo_BOV"))
        {
            trigger_source = SS_TRIG_TURBOBOV;
            return true;
        };
        if (vec[1] == String("turbo_waste_gate"))
        {
            trigger_source = SS_TRIG_TURBOWASTEGATE;
            return true;
        };
        if (vec[1] == String("turbo_back_fire"))
        {
            trigger_source = SS_TRIG_TURBOBACKFIRE;
            return true;
        };
        if (vec[1] == String("always_on"))
        {
            trigger_source = SS_TRIG_ALWAYSON;
            return true;
        };
        if (vec[1] == String("repair"))
        {
            trigger_source = SS_TRIG_REPAIR;
            return true;
        };
        if (vec[1] == String("air"))
        {
            trigger_source = SS_TRIG_AIR;
            return true;
        };
        if (vec[1] == String("gpws_ap_disconnect"))
        {
            trigger_source = SS_TRIG_GPWS_APDISCONNECT;
            return true;
        };
        if (vec[1] == String("gpws_10"))
        {
            trigger_source = SS_TRIG_GPWS_10;
            return true;
        };
        if (vec[1] == String("gpws_20"))
        {
            trigger_source = SS_TRIG_GPWS_20;
            return true;
        };
        if (vec[1] == String("gpws_30"))
        {
            trigger_source = SS_TRIG_GPWS_30;
            return true;
        };
        if (vec[1] == String("gpws_40"))
        {
            trigger_source = SS_TRIG_GPWS_40;
            return true;
        };
        if (vec[1] == String("gpws_50"))
        {
            trigger_source = SS_TRIG_GPWS_50;
            return true;
        };
        if (vec[1] == String("gpws_100"))
        {
            trigger_source = SS_TRIG_GPWS_100;
            return true;
        };
        if (vec[1] == String("gpws_pull_up"))
        {
            trigger_source = SS_TRIG_GPWS_PULLUP;
            return true;
        };
        if (vec[1] == String("gpws_minimums"))
        {
            trigger_source = SS_TRIG_GPWS_MINIMUMS;
            return true;
        };
        if (vec[1] == String("air_purge"))
        {
            trigger_source = SS_TRIG_AIR_PURGE;
            return true;
        };
        if (vec[1] == String("shift"))
        {
            trigger_source = SS_TRIG_SHIFT;
            return true;
        };
        if (vec[1] == String("gear_slide"))
        {
            trigger_source = SS_TRIG_GEARSLIDE;
            return true;
        };
        if (vec[1] == String("creak") && App::audio_enable_creak.GetActive())
        {
            trigger_source = SS_TRIG_CREAK;
            return true;
        };
        if (vec[1] == String("break"))
        {
            trigger_source = SS_TRIG_BREAK;
            return true;
        };
        if (vec[1] == String("screetch"))
        {
            trigger_source = SS_TRIG_SCREETCH;
            return true;
        };
        if (vec[1] == String("parking_brake"))
        {
            trigger_source = SS_TRIG_PARK;
            return true;
        };
        if (vec[1] == String("antilock"))
        {
            trigger_source = SS_TRIG_ALB_ACTIVE;
            return true;
        };
        if (vec[1] == String("tractioncontrol"))
        {
            trigger_source = SS_TRIG_TC_ACTIVE;
            return true;
        };
        if (vec[1] == String("afterburner1"))
        {
            trigger_source = SS_TRIG_AFTERBURNER1;
            return true;
        };
        if (vec[1] == String("afterburner2"))
        {
            trigger_source = SS_TRIG_AFTERBURNER2;
            return true;
        };
        if (vec[1] == String("afterburner3"))
        {
            trigger_source = SS_TRIG_AFTERBURNER3;
            return true;
        };
        if (vec[1] == String("afterburner4"))
        {
            trigger_source = SS_TRIG_AFTERBURNER4;
            return true;
        };
        if (vec[1] == String("afterburner5"))
        {
            trigger_source = SS_TRIG_AFTERBURNER5;
            return true;
        };
        if (vec[1] == String("afterburner6"))
        {
            trigger_source = SS_TRIG_AFTERBURNER6;
            return true;
        };
        if (vec[1] == String("afterburner7"))
        {
            trigger_source = SS_TRIG_AFTERBURNER7;
            return true;
        };
        if (vec[1] == String("afterburner8"))
        {
            trigger_source = SS_TRIG_AFTERBURNER8;
            return true;
        };
        if (vec[1] == String("avionic_chat_01"))
        {
            trigger_source = SS_TRIG_AVICHAT01;
            return true;
        };
        if (vec[1] == String("avionic_chat_02"))
        {
            trigger_source = SS_TRIG_AVICHAT02;
            return true;
        };
        if (vec[1] == String("avionic_chat_03"))
        {
            trigger_source = SS_TRIG_AVICHAT03;
            return true;
        };
        if (vec[1] == String("avionic_chat_04"))
        {
            trigger_source = SS_TRIG_AVICHAT04;
            return true;
        };
        if (vec[1] == String("avionic_chat_05"))
        {
            trigger_source = SS_TRIG_AVICHAT05;
            return true;
        };
        if (vec[1] == String("avionic_chat_06"))
        {
            trigger_source = SS_TRIG_AVICHAT06;
            return true;
        };
        if (vec[1] == String("avionic_chat_07"))
        {
            trigger_source = SS_TRIG_AVICHAT07;
            return true;
        };
        if (vec[1] == String("avionic_chat_08"))
        {
            trigger_source = SS_TRIG_AVICHAT08;
            return true;
        };
        if (vec[1] == String("avionic_chat_09"))
        {
            trigger_source = SS_TRIG_AVICHAT09;
            return true;
        };
        if (vec[1] == String("avionic_chat_10"))
        {
            trigger_source = SS_TRIG_AVICHAT10;
            return true;
        };
        if (vec[1] == String("avionic_chat_11"))
        {
            trigger_source = SS_TRIG_AVICHAT11;
            return true;
        };
        if (vec[1] == String("avionic_chat_12"))
        {
            trigger_source = SS_TRIG_AVICHAT12;
            return true;
        };
        if (vec[1] == String("avionic_chat_13"))
        {
            trigger_source = SS_TRIG_AVICHAT13;
            return true;
        };
        if (vec[1] == String("aoa_horn"))
        {
            trigger_source = SS_TRIG_AOA;
            return true;
        };
        if (vec[1] == String("ignition"))
        {
            trigger_source = SS_TRIG_IGNITION;
            return true;
        };
        if (vec[1] == String("reverse_gear"))
        {
            trigger_source = SS_TRIG_REVERSE_GEAR;
            return true;
        };
        if (vec[1] == String("turn_signal"))
        {
            trigger_source = SS_TRIG_TURN_SIGNAL;
            return true;
        };
        if (vec[1] == String("turn_signal_tick"))
        {
            trigger_source = SS_TRIG_TURN_SIGNAL_TICK;
            return true;
        };
        if (vec[1] == String("turn_signal_warn_tick"))
        {
            trigger_source = SS_TRIG_TURN_SIGNAL_WARN_TICK;
            return true;
        };
        if (vec[1] == String("linked_command"))
        {
            trigger_source = SS_TRIG_LINKED_COMMAND;
            return true;
        };
        if (vec[1] == String("main_menu"))
        {
            trigger_source = SS_TRIG_MAIN_MENU;
            return true;
        };

        return false;
    }

    if (vec[0] == String("pitch_source"))
    {
        if (vec.size() < 2)
            return false;
        int mod = parseModulation(vec[1]);
        if (mod >= 0)
        {
            pitch_source = mod;
            return true;
        }
        return false;
    }

    if (vec[0] == String("pitch_factors"))
    {
        if (vec.size() < 3)
            return false;
        pitch_offset = StringConverter::parseReal(vec[1]);
        pitch_multiplier = StringConverter::parseReal(vec[2]);
        if (vec.size() == 4)
        {
            pitch_square = StringConverter::parseReal(vec[3]);
        }
        return true;
    }

    if (vec[0] == String("gain_source"))
    {
        if (vec.size() < 2)
            return false;
        int mod = parseModulation(vec[1]);
        if (mod >= 0)
        {
            gain_source = mod;
            return true;
        }
        return false;
    }

    if (vec[0] == String("gain_factors"))
    {
        if (vec.size() < 3)
            return false;
        gain_offset = StringConverter::parseReal(vec[1]);
        gain_multiplier = StringConverter::parseReal(vec[2]);
        if (vec.size() == 4)
        {
            gain_square = StringConverter::parseReal(vec[3]);
        }
        return true;
    }

    if (vec[0] == String("start_sound"))
    {
        if (vec.size() < 3)
            return false;
        start_sound_pitch = StringConverter::parseReal(vec[1]); // unparsable (e.g. "unpitched") will result in value 0.0
        start_sound_name = vec[2];
        has_start_sound = true;
        return true;
    }

    if (vec[0] == String("stop_sound"))
    {
        if (vec.size() < 3)
            return false;
        stop_sound_pitch = StringConverter::parseReal(vec[1]); // unparsable (e.g. "unpitched") will result in value 0.0
        stop_sound_name = vec[2];
        has_stop_sound = true;
        return true;
    }

    if (vec[0] == String("sound"))
    {
        if (vec.size() < 3)
            return false;
        if (free_sound >= MAX_SOUNDS_PER_SCRIPT)
        {
            LOG("SoundScriptManager: Reached MAX_SOUNDS_PER_SCRIPT limit (" + TOSTRING(MAX_SOUNDS_PER_SCRIPT) + ")");
            return false;
        }
        sound_pitches[free_sound] = StringConverter::parseReal(vec[1]); // unparsable (e.g. "unpitched") will result in value 0.0
        if (sound_pitches[free_sound] == 0)
        {
            unpitchable = true;
        }
        if (free_sound > 0 && !unpitchable && sound_pitches[free_sound] <= sound_pitches[free_sound - 1])
        {
            return false;
        }
        sound_names[free_sound] = vec[2];
        free_sound++;
        return true;
    }

    return false;
}

int SoundScriptTemplate::parseModulation(String str)
{
    if (str == String("none"))
        return SS_MOD_NONE;
    if (str == String("engine_rpm"))
        return SS_MOD_ENGINE;
    if (str == String("turbo_rpm"))
        return SS_MOD_TURBO;
    if (str == String("aeroengine1_rpm"))
        return SS_MOD_AEROENGINE1;
    if (str == String("aeroengine2_rpm"))
        return SS_MOD_AEROENGINE2;
    if (str == String("aeroengine3_rpm"))
        return SS_MOD_AEROENGINE3;
    if (str == String("aeroengine4_rpm"))
        return SS_MOD_AEROENGINE4;
    if (str == String("aeroengine5_rpm"))
        return SS_MOD_AEROENGINE5;
    if (str == String("aeroengine6_rpm"))
        return SS_MOD_AEROENGINE6;
    if (str == String("aeroengine7_rpm"))
        return SS_MOD_AEROENGINE7;
    if (str == String("aeroengine8_rpm"))
        return SS_MOD_AEROENGINE8;
    if (str == String("wheel_speed_kmph"))
        return SS_MOD_WHEELSPEED;
    if (str == String("injector_ratio"))
        return SS_MOD_INJECTOR;
    if (str == String("torque_nm"))
        return SS_MOD_TORQUE;
    if (str == String("gearbox_rpm"))
        return SS_MOD_GEARBOX;
    if (str == String("creak"))
        return SS_MOD_CREAK;
    if (str == String("break"))
        return SS_MOD_BREAK;
    if (str == String("screetch"))
        return SS_MOD_SCREETCH;
    if (str == String("pump_rpm"))
        return SS_MOD_PUMP;
    if (str == String("aeroengine1_throttle"))
        return SS_MOD_THROTTLE1;
    if (str == String("aeroengine2_throttle"))
        return SS_MOD_THROTTLE2;
    if (str == String("aeroengine3_throttle"))
        return SS_MOD_THROTTLE3;
    if (str == String("aeroengine4_throttle"))
        return SS_MOD_THROTTLE4;
    if (str == String("aeroengine5_throttle"))
        return SS_MOD_THROTTLE5;
    if (str == String("aeroengine6_throttle"))
        return SS_MOD_THROTTLE6;
    if (str == String("aeroengine7_throttle"))
        return SS_MOD_THROTTLE7;
    if (str == String("aeroengine8_throttle"))
        return SS_MOD_THROTTLE8;
    if (str == String("air_speed_knots"))
        return SS_MOD_AIRSPEED;
    if (str == String("angle_of_attack_degree"))
        return SS_MOD_AOA;
    if (str == String("linked_command_rate"))
        return SS_MOD_LINKED_COMMANDRATE;
    if (str == String("music_volume"))
        return SS_MOD_MUSIC_VOLUME;

    return -1;
}

//====================================================================

SoundScriptInstance::SoundScriptInstance(int actor_id, SoundScriptTemplate* templ, SoundManager* sound_manager, String instancename, int soundLinkType, int soundLinkItemId) :
    actor_id(actor_id)
    , templ(templ)
    , sound_manager(sound_manager)
    , sound_link_type(soundLinkType)
    , sound_link_item_id(soundLinkItemId)
    , start_sound(NULL)
    , start_sound_pitchgain(0.0f)
    , stop_sound(NULL)
    , stop_sound_pitchgain(0.0f)
    , lastgain(1.0f)
{
    // create sounds
    if (templ->has_start_sound)
    {
        start_sound = sound_manager->createSound(templ->start_sound_name);
    }

    if (templ->has_stop_sound)
    {
        stop_sound = sound_manager->createSound(templ->stop_sound_name);
    }

    for (int i = 0; i < templ->free_sound; i++)
    {
        sounds[i] = sound_manager->createSound(templ->sound_names[i]);
    }

    setPitch(0.0f);
    setGain(1.0f);

    LOG("SoundScriptInstance: instance created: "+instancename);
}

void SoundScriptInstance::setPitch(float value)
{
    if (start_sound)
    {
        start_sound_pitchgain = pitchgain_cutoff(templ->start_sound_pitch, value);

        if (start_sound_pitchgain != 0.0f && templ->start_sound_pitch != 0.0f)
        {
            start_sound->setPitch(value / templ->start_sound_pitch);
        }
    }

    if (templ->free_sound)
    {
        // searching the interval
        int up = 0;

        for (up = 0; up < templ->free_sound; up++)
        {
            if (templ->sound_pitches[up] > value)
            {
                break;
            }
        }

        if (up == 0)
        {
            // low sound case
            sounds_pitchgain[0] = pitchgain_cutoff(templ->sound_pitches[0], value);

            if (sounds_pitchgain[0] != 0.0f && templ->sound_pitches[0] != 0.0f && sounds[0])
            {
                sounds[0]->setPitch(value / templ->sound_pitches[0]);
            }

            for (int i = 1; i < templ->free_sound; i++)
            {
                if (templ->sound_pitches[i] != 0.0f)
                {
                    sounds_pitchgain[i] = 0.0f;
                    // pause?
                }
                else
                {
                    sounds_pitchgain[i] = 1.0f; // unpitched
                }
            }
        }
        else if (up == templ->free_sound)
        {
            // high sound case
            for (int i = 0; i < templ->free_sound - 1; i++)
            {
                if (templ->sound_pitches[i] != 0.0f)
                {
                    sounds_pitchgain[i] = 0.0f;
                    // pause?
                }
                else
                {
                    sounds_pitchgain[i] = 1.0f; // unpitched
                }
            }

            sounds_pitchgain[templ->free_sound - 1] = 1.0f;

            if (templ->sound_pitches[templ->free_sound - 1] != 0.0f && sounds[templ->free_sound - 1])
            {
                sounds[templ->free_sound - 1]->setPitch(value / templ->sound_pitches[templ->free_sound - 1]);
            }
        }
        else
        {
            // middle sound case
            int low = up - 1;

            for (int i = 0; i < low; i++)
            {
                if (templ->sound_pitches[i] != 0.0f)
                {
                    sounds_pitchgain[i] = 0.0f;
                    // pause?
                }
                else
                {
                    sounds_pitchgain[i] = 1.0f; // unpitched
                }
            }

            if (templ->sound_pitches[low] != 0.0f && sounds[low])
            {
                sounds_pitchgain[low] = (templ->sound_pitches[up] - value) / (templ->sound_pitches[up] - templ->sound_pitches[low]);
                sounds[low]->setPitch(value / templ->sound_pitches[low]);
            }
            else
            {
                sounds_pitchgain[low] = 1.0f; // unpitched
            }

            if (templ->sound_pitches[up] != 0.0f && sounds[up])
            {
                sounds_pitchgain[up] = (value - templ->sound_pitches[low]) / (templ->sound_pitches[up] - templ->sound_pitches[low]);
                sounds[up]->setPitch(value / templ->sound_pitches[up]);
            }
            else
            {
                sounds_pitchgain[up] = 1.0f; // unpitched
            }

            for (int i = up + 1; i < templ->free_sound; i++)
            {
                if (templ->sound_pitches[i] != 0.0f)
                {
                    sounds_pitchgain[i] = 0.0f;
                    // pause?
                }
                else
                {
                    sounds_pitchgain[i] = 1.0f; // unpitched
                }
            }
        }
    }

    if (stop_sound)
    {
        stop_sound_pitchgain = pitchgain_cutoff(templ->stop_sound_pitch, value);

        if (stop_sound_pitchgain != 0.0f && templ->stop_sound_pitch != 0.0f)
        {
            stop_sound->setPitch(value / templ->stop_sound_pitch);
        }
    }

    // propagate new gains
    setGain(lastgain);
}

float SoundScriptInstance::pitchgain_cutoff(float sourcepitch, float targetpitch)
{
    if (sourcepitch == 0.0f)
    {
        return 1.0f; // unpitchable
    }

    if (targetpitch > sourcepitch / PITCHDOWN_FADE_FACTOR)
    {
        return 1.0f; // pass
    }

    if (targetpitch < sourcepitch / PITCHDOWN_CUTOFF_FACTOR)
    {
        return 0.0f; // cutoff
    }

    // linear fading
    return (targetpitch - sourcepitch / PITCHDOWN_CUTOFF_FACTOR) / (sourcepitch / PITCHDOWN_FADE_FACTOR - sourcepitch / PITCHDOWN_CUTOFF_FACTOR);
}

void SoundScriptInstance::setGain(float value)
{
    if (start_sound)
    {
        start_sound->setGain(value * start_sound_pitchgain);
    }

    for (int i = 0; i < templ->free_sound; i++)
    {
        if (sounds[i])
        {
            sounds[i]->setGain(value * sounds_pitchgain[i]);
        }
    }

    if (stop_sound)
    {
        stop_sound->setGain(value * stop_sound_pitchgain);
    }

    lastgain = value;
}

void SoundScriptInstance::setPosition(Vector3 pos, Vector3 velocity)
{
    if (start_sound)
    {
        start_sound->setPosition(pos);
        start_sound->setVelocity(velocity);
    }

    for (int i = 0; i < templ->free_sound; i++)
    {
        if (sounds[i])
        {
            sounds[i]->setPosition(pos);
            sounds[i]->setVelocity(velocity);
        }
    }

    if (stop_sound)
    {
        stop_sound->setPosition(pos);
        stop_sound->setVelocity(velocity);
    }
}

void SoundScriptInstance::runOnce()
{
    if (start_sound)
    {
        if (start_sound->isPlaying())
        {
            return;
        }
        start_sound->play();
    }

    for (int i = 0; i < templ->free_sound; i++)
    {
        if (sounds[i])
        {
            if (sounds[i]->isPlaying())
            {
                continue;
            }
            sounds[i]->setLoop(false);
            sounds[i]->play();
        }
    }

    if (stop_sound)
    {
        if (stop_sound->isPlaying())
        {
            return;
        }
        stop_sound->play();
    }
}

void SoundScriptInstance::start()
{
    if (start_sound)
    {
        start_sound->stop();
        //start_sound->setLoop(true);
        start_sound->play();
    }

    for (int i = 0; i < templ->free_sound; i++)
    {
        if (sounds[i])
        {
            sounds[i]->setLoop(true);
            sounds[i]->play();
        }
    }
}

void SoundScriptInstance::stop()
{
    for (int i = 0; i < templ->free_sound; i++)
    {
        if (sounds[i])
            sounds[i]->stop();
    }

    if (stop_sound)
    {
        stop_sound->stop();
        stop_sound->play();
    }
}

void SoundScriptInstance::kill()
{
    for (int i = 0; i < templ->free_sound; i++)
    {
        if (sounds[i])
            sounds[i]->stop();
    }

    if (start_sound)
        start_sound->stop();

    if (stop_sound)
    {
        stop_sound->stop();
        stop_sound->play();
    }
}

void SoundScriptInstance::setEnabled(bool e)
{
    if (start_sound)
    {
        start_sound->setEnabled(e);
    }

    if (stop_sound)
    {
        stop_sound->setEnabled(e);
    }

    for (int i = 0; i < templ->free_sound; i++)
    {
        if (sounds[i])
        {
            sounds[i]->setEnabled(e);
        }
    }
}

#endif // USE_OPENAL
