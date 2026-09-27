/*
    This source file is part of Rigs of Rods
    Copyright 2023 Petr Ohlidal

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

/// @file
/// @author Petr Ohlidal

#include "../../autowrapper/aswrappedcall.h"
#include "AngelScriptBindings.h"
#include "ScriptEngine.h"
#include "Sound.h"
#include "SoundScriptManager.h"
#include "wrappers/SoundScriptAngelscriptWrappers.h"

using namespace RoR;
using namespace AngelScript;
using namespace SoundScriptAngelscriptWrappers;

void RoR::RegisterSoundScriptGeneric(AngelScript::asIScriptEngine* engine)
{
    int result = 0;

    // NOTE: enums SoundTriggers and ModulationSources are registered in RegisterSoundScriptCommon()

    // class SoundScriptTemplate
    SoundScriptTemplate::RegisterRefCountingObjectGeneric(engine, "SoundScriptTemplateClass");
    SoundScriptTemplatePtr::RegisterRefCountingObjectPtrGeneric(engine, "SoundScriptTemplateClassPtr", "SoundScriptTemplateClass");

    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "int getNumSounds()", WRAP_MFN(SoundScriptTemplate, getNumSounds), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "string getSoundName(int)", WRAP_MFN(SoundScriptTemplate, getSoundName), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "float getSoundPitch(int)", WRAP_MFN(SoundScriptTemplate, getSoundPitch), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "string getStartSoundName()", WRAP_MFN(SoundScriptTemplate, getStartSoundName), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "float getStartSoundPitch()", WRAP_MFN(SoundScriptTemplate, getStartSoundPitch), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "string getStopSoundName()", WRAP_MFN(SoundScriptTemplate, getStopSoundName), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "float getStopSoundPitch()", WRAP_MFN(SoundScriptTemplate, getStopSoundPitch), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "string getName()", WRAP_MFN(SoundScriptTemplate, getName), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "string getFileName()", WRAP_MFN(SoundScriptTemplate, getFileName), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "string getGroupName()", WRAP_MFN(SoundScriptTemplate, getGroupName), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "bool isBaseTemplate()", WRAP_MFN(SoundScriptTemplate, isBaseTemplate), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    // class Sound
    Sound::RegisterRefCountingObjectGeneric(engine, "SoundClass");
    SoundPtr::RegisterRefCountingObjectPtrGeneric(engine, "SoundClassPtr", "SoundClass");

    result = engine->RegisterObjectMethod("SoundClass", "void setPitch(float pitch)", WRAP_MFN(Sound, setPitch), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "void setGain(float gain)", WRAP_MFN(Sound, setGain), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "void setPosition(vector3 pos)", WRAP_MFN(Sound, setPosition), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "void setVelocity(vector3 vel)", WRAP_MFN(Sound, setVelocity), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "void setLoop(bool loop)", WRAP_MFN(Sound, setLoop), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "void setEnabled(bool e)", WRAP_MFN(Sound, setEnabled), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "void play()", WRAP_MFN(Sound, play), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "void stop()", WRAP_MFN(Sound, stop), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("SoundClass", "bool getEnabled()", WRAP_MFN(Sound, getEnabled), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "bool isPlaying()", WRAP_MFN(Sound, isPlaying), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "float getAudibility() ", WRAP_MFN(Sound, getAudibility), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "float getGain() ", WRAP_MFN(Sound, getGain), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "float getPitch() ", WRAP_MFN(Sound, getPitch), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "bool getLoop()", WRAP_MFN(Sound, getLoop), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "int getCurrentHardwareIndex()", WRAP_MFN(Sound, getCurrentHardwareIndex), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "uint getBuffer()", WRAP_MFN(Sound, getBuffer), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "vector3 getPosition()", WRAP_MFN(Sound, getPosition), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "vector3 getVelocity()", WRAP_MFN(Sound, getVelocity), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "int getSourceIndex()", WRAP_MFN(Sound, getSourceIndex), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    // class SoundScriptInstance
    SoundScriptInstance::RegisterRefCountingObjectGeneric(engine, "SoundScriptInstanceClass");
    SoundScriptInstancePtr::RegisterRefCountingObjectPtrGeneric(engine, "SoundScriptInstanceClassPtr", "SoundScriptInstanceClass");

    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "void runOnce()", WRAP_MFN(SoundScriptInstance, runOnce), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "void setPitch(float pitch)", WRAP_MFN(SoundScriptInstance, setPitch), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "void setGain(float gain)", WRAP_MFN(SoundScriptInstance, setGain), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "void setPosition(vector3 pos)", WRAP_MFN(SoundScriptInstance, setPosition), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "void setVelocity(vector3 velo)", WRAP_MFN(SoundScriptInstance, setVelocity), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "void start()", WRAP_MFN(SoundScriptInstance, start), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "void stop()", WRAP_MFN(SoundScriptInstance, stop), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "void kill()", WRAP_MFN(SoundScriptInstance, kill), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "SoundScriptTemplateClassPtr@ getTemplate()", WRAP_OBJ_FIRST(SoundScriptInstance_getTemplate), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "SoundClassPtr@ getStartSound()", WRAP_OBJ_FIRST(SoundScriptInstance_getStartSound), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "SoundClassPtr@ getStopSound()", WRAP_OBJ_FIRST(SoundScriptInstance_getStopSound), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "SoundClassPtr@ getSound(int pos)", WRAP_OBJ_FIRST(SoundScriptInstance_getSound), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "float getStartSoundPitchgain()", WRAP_MFN(SoundScriptInstance, getStartSoundPitchgain), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "float getStopSoundPitchgain()", WRAP_MFN(SoundScriptInstance, getStopSoundPitchgain), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "float getSoundPitchgain(int pos)", WRAP_MFN(SoundScriptInstance, getSoundPitchgain), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "int getActorInstanceId()", WRAP_MFN(SoundScriptInstance, getActorInstanceId), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "const string& getInstanceName()", WRAP_MFN(SoundScriptInstance, getInstanceName), asCALL_GENERIC); ROR_ASSERT(result >= 0);

}
