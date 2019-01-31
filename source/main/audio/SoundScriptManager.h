/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2017-2018 Petr Ohlidal

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

#pragma once

#include "RoRPrerequisites.h"

#include "Singleton.h"

#include <OgreScriptLoader.h>

#define SOUND_PLAY_ONCE(_ACTOR_, _TRIG_)        SoundScriptManager::getSingleton().trigOnce    ( (_ACTOR_), (_TRIG_) )
#define SOUND_START(_ACTOR_, _TRIG_)            SoundScriptManager::getSingleton().trigStart   ( (_ACTOR_), (_TRIG_) )
#define SOUND_STOP(_ACTOR_, _TRIG_)             SoundScriptManager::getSingleton().trigStop    ( (_ACTOR_), (_TRIG_) )
#define SOUND_TOGGLE(_ACTOR_, _TRIG_)           SoundScriptManager::getSingleton().trigToggle  ( (_ACTOR_), (_TRIG_) )
#define SOUND_KILL(_ACTOR_, _TRIG_)             SoundScriptManager::getSingleton().trigKill    ( (_ACTOR_), (_TRIG_) )
#define SOUND_GET_STATE(_ACTOR_, _TRIG_)        SoundScriptManager::getSingleton().getTrigState( (_ACTOR_), (_TRIG_) )
#define SOUND_MODULATE(_ACTOR_, _MOD_, _VALUE_) SoundScriptManager::getSingleton().modulate    ( (_ACTOR_), (_MOD_), (_VALUE_) )

enum {
    MAX_SOUNDS_PER_SCRIPT = 16,
    MAX_INSTANCES_PER_GROUP = 256
};

enum SoundTriggers {
    SS_TRIG_NONE = -1,
    SS_TRIG_ENGINE = 0,
    SS_TRIG_AEROENGINE1,
    SS_TRIG_AEROENGINE2,
    SS_TRIG_AEROENGINE3,
    SS_TRIG_AEROENGINE4,
    SS_TRIG_HORN,
    SS_TRIG_BRAKE,
    SS_TRIG_PUMP,
    SS_TRIG_STARTER,
    SS_TRIG_TURBOBOV,
    SS_TRIG_TURBOWASTEGATE,
    SS_TRIG_TURBOBACKFIRE,
    SS_TRIG_ALWAYSON,
    SS_TRIG_REPAIR,
    SS_TRIG_AIR,
    SS_TRIG_GPWS_APDISCONNECT,
    SS_TRIG_GPWS_10,
    SS_TRIG_GPWS_20,
    SS_TRIG_GPWS_30,
    SS_TRIG_GPWS_40,
    SS_TRIG_GPWS_50,
    SS_TRIG_GPWS_100,
    SS_TRIG_GPWS_PULLUP,
    SS_TRIG_GPWS_MINIMUMS,
    SS_TRIG_AIR_PURGE,
    SS_TRIG_SHIFT,
    SS_TRIG_GEARSLIDE,
    SS_TRIG_CREAK,
    SS_TRIG_BREAK,
    SS_TRIG_SCREETCH,
    SS_TRIG_PARK,
    SS_TRIG_AFTERBURNER1,
    SS_TRIG_AFTERBURNER2,
    SS_TRIG_AFTERBURNER3,
    SS_TRIG_AFTERBURNER4,
    SS_TRIG_AFTERBURNER5,
    SS_TRIG_AFTERBURNER6,
    SS_TRIG_AFTERBURNER7,
    SS_TRIG_AFTERBURNER8,
    SS_TRIG_AEROENGINE5,
    SS_TRIG_AEROENGINE6,
    SS_TRIG_AEROENGINE7,
    SS_TRIG_AEROENGINE8,
    SS_TRIG_AOA,
    SS_TRIG_IGNITION,
    SS_TRIG_REVERSE_GEAR,
    SS_TRIG_TURN_SIGNAL,
    SS_TRIG_TURN_SIGNAL_TICK,
    SS_TRIG_TURN_SIGNAL_WARN_TICK,
    SS_TRIG_ALB_ACTIVE,
    SS_TRIG_TC_ACTIVE,
    SS_TRIG_AVICHAT01,
    SS_TRIG_AVICHAT02,
    SS_TRIG_AVICHAT03,
    SS_TRIG_AVICHAT04,
    SS_TRIG_AVICHAT05,
    SS_TRIG_AVICHAT06,
    SS_TRIG_AVICHAT07,
    SS_TRIG_AVICHAT08,
    SS_TRIG_AVICHAT09,
    SS_TRIG_AVICHAT10,
    SS_TRIG_AVICHAT11,
    SS_TRIG_AVICHAT12,
    SS_TRIG_AVICHAT13,
    SS_TRIG_LINKED_COMMAND,
    SS_TRIG_MAIN_MENU,
    SS_MAX_TRIG
};

enum ModulationSources {
    SS_MOD_NONE,
    SS_MOD_ENGINE,
    SS_MOD_TURBO,
    SS_MOD_AEROENGINE1,
    SS_MOD_AEROENGINE2,
    SS_MOD_AEROENGINE3,
    SS_MOD_AEROENGINE4,
    SS_MOD_WHEELSPEED,
    SS_MOD_INJECTOR,
    SS_MOD_TORQUE,
    SS_MOD_GEARBOX,
    SS_MOD_CREAK,
    SS_MOD_BREAK,
    SS_MOD_SCREETCH,
    SS_MOD_PUMP,
    SS_MOD_THROTTLE1,
    SS_MOD_THROTTLE2,
    SS_MOD_THROTTLE3,
    SS_MOD_THROTTLE4,
    SS_MOD_THROTTLE5,
    SS_MOD_THROTTLE6,
    SS_MOD_THROTTLE7,
    SS_MOD_THROTTLE8,
    SS_MOD_AEROENGINE5,
    SS_MOD_AEROENGINE6,
    SS_MOD_AEROENGINE7,
    SS_MOD_AEROENGINE8,
    SS_MOD_AIRSPEED,
    SS_MOD_AOA,
    SS_MOD_LINKED_COMMANDRATE,
    SS_MOD_MUSIC_VOLUME,
    SS_MAX_MOD
};

enum SoundLinkTypes {
    SL_DEFAULT,
    SL_COMMAND, 
    SL_HYDRO, 
    SL_COLLISION, 
    SL_SHOCKS, 
    SL_BRAKES, 
    SL_ROPES, 
    SL_TIES, 
    SL_PARTICLES, 
    SL_AXLES, 
    SL_FLARES, 
    SL_FLEXBODIES, 
    SL_EXHAUSTS, 
    SL_VIDEOCAMERA, 
    SL_MAX
};

class Sound;
class SoundManager;
class MumbleIntegration;

class SoundScriptTemplate : public ZeroedMemoryAllocator
{
    friend class SoundScriptManager;
    friend class SoundScriptInstance;

public:

    SoundScriptTemplate(Ogre::String name, Ogre::String groupname, Ogre::String filename, bool baseTemplate);
    
private:

    int parseModulation(Ogre::String str);
    bool setParameter(Ogre::StringVector vec);

    Ogre::String name;
    Ogre::String file_name;

    bool         base_template;
    bool         has_start_sound;
    bool         has_stop_sound;
    bool         unpitchable;

    float        gain_multiplier;
    float        gain_offset;
    float        gain_square;
    int          gain_source;

    float        pitch_multiplier;
    float        pitch_offset;
    float        pitch_square;
    int          pitch_source;

    Ogre::String sound_names[MAX_SOUNDS_PER_SCRIPT];
    float        sound_pitches[MAX_SOUNDS_PER_SCRIPT];
    Ogre::String start_sound_name;
    float        start_sound_pitch;
    Ogre::String stop_sound_name;
    float        stop_sound_pitch;

    int          trigger_source;
    int          free_sound;
};

class SoundScriptInstance : public ZeroedMemoryAllocator
{
    friend class SoundScriptManager;
    friend class RigInspector;

public:

    SoundScriptInstance(int actor_id, SoundScriptTemplate* templ, SoundManager* sm, Ogre::String instancename, int soundLinkType=SL_DEFAULT, int soundLinkItemId=-1);
    void runOnce();
    void setEnabled(bool e);
    void setGain(float value);
    void setPitch(float value);
    void setPosition(Ogre::Vector3 pos, Ogre::Vector3 velocity);
    void start();
    void stop();
    void kill();

    static const float PITCHDOWN_FADE_FACTOR;
    static const float PITCHDOWN_CUTOFF_FACTOR;

private:

    float pitchgain_cutoff(float sourcepitch, float targetpitch);

    SoundScriptTemplate* templ;
    SoundManager* sound_manager;
    Sound *start_sound;
    Sound *stop_sound;
    Sound *sounds[MAX_SOUNDS_PER_SCRIPT];
    float start_sound_pitchgain;
    float stop_sound_pitchgain;
    float sounds_pitchgain[MAX_SOUNDS_PER_SCRIPT];
    float lastgain;

    int actor_id;           // ID of the actor this sound belongs to.
    int sound_link_type;    // holds the SL_ type this is bound to
    int sound_link_item_id; // holds the item number this is for
};

class SoundScriptManager : public Ogre::ScriptLoader, public RoRSingleton<SoundScriptManager>, public ZeroedMemoryAllocator
{
public:

    SoundScriptManager();
    ~SoundScriptManager();

    // ScriptLoader interface
    const Ogre::StringVector& getScriptPatterns(void) const;
    void parseScript(Ogre::DataStreamPtr& stream, const Ogre::String& groupName);
    Ogre::Real getLoadingOrder(void) const;

    SoundScriptInstance* createInstance(Ogre::String templatename, int actor_id, Ogre::SceneNode *toAttach=NULL, int soundLinkType=SL_DEFAULT, int soundLinkItemId=-1);

    // functions
    void trigOnce    (int actor_id, int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
    void trigOnce    (Actor* actor, int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
    void trigStart   (int actor_id, int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
    void trigStart   (Actor* actor, int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
    void trigStop    (int actor_id, int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
    void trigStop    (Actor* actor, int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
    void trigToggle  (int actor_id, int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
    void trigToggle  (Actor* actor, int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
    void trigKill	 (int actor_id, int trig, int linkType = SL_DEFAULT, int linkItemID = -1);
    void trigKill    (Actor* actor, int trig, int linkType = SL_DEFAULT, int linkItemID = -1);
    bool getTrigState(int actor_id, int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
    bool getTrigState(Actor* actor, int trig, int linkType = SL_DEFAULT, int linkItemID=-1);
    void modulate    (int actor_id, int mod, float value, int linkType = SL_DEFAULT, int linkItemID=-1);
    void modulate    (Actor* actor, int mod, float value, int linkType = SL_DEFAULT, int linkItemID=-1);

    void setEnabled(bool state);

    void setCamera(Ogre::Vector3 position, Ogre::Vector3 direction, Ogre::Vector3 up, Ogre::Vector3 velocity);
    void setLoadingBaseSounds(bool value) { loading_base = value; };

    bool isDisabled() { return disabled; }

private:

    SoundScriptTemplate* createTemplate(Ogre::String name, Ogre::String groupname, Ogre::String filename);
    void skipToNextCloseBrace(Ogre::DataStreamPtr& chunk);
    void skipToNextOpenBrace(Ogre::DataStreamPtr& chunk);

    bool disabled;
    bool loading_base;
    float max_distance;
    float reference_distance;
    float rolloff_factor;
    int instance_counter;
    Ogre::StringVector script_patterns;

    std::map <Ogre::String, SoundScriptTemplate*> templates;

    // instances lookup tables
    int free_trigs[SS_MAX_TRIG];
    SoundScriptInstance *trigs[SS_MAX_TRIG * MAX_INSTANCES_PER_GROUP];

    int free_pitches[SS_MAX_MOD];
    SoundScriptInstance *pitches[SS_MAX_MOD * MAX_INSTANCES_PER_GROUP];
    
    int free_gains[SS_MAX_MOD];
    SoundScriptInstance *gains[SS_MAX_MOD * MAX_INSTANCES_PER_GROUP];

    // state map
    // soundLinks, soundItems, actor_ids, triggers
    std::map <int, std::map <int, std::map <int, std::map <int, bool > > > > state_map;

    SoundManager* sound_manager;
};

#else // USE_OPENAL

#define SOUND_PLAY_ONCE(_ACTOR_, _TRIG_)
#define SOUND_START(_ACTOR_, _TRIG_)
#define SOUND_STOP(_ACTOR_, _TRIG_)
#define SOUND_TOGGLE(_ACTOR_, _TRIG_)
#define SOUND_KILL(_ACTOR_, _TRIG_)
#define SOUND_GET_STATE(_ACTOR_, _TRIG_) (false)
#define SOUND_MODULATE(_ACTOR_, _MOD_, _VALUE_)

#endif // USE_OPENAL
