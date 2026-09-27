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

#include "AngelScriptBindings.h"
#include "ScriptEngine.h"
#include "Sound.h"
#include "SoundScriptManager.h"
#include "wrappers/SoundScriptAngelscriptWrappers.h"

using namespace RoR;
using namespace AngelScript;
using namespace SoundScriptAngelscriptWrappers;

void RoR::RegisterSoundScriptNative(AngelScript::asIScriptEngine* engine)
{
    int result = 0;

    // NOTE: enums SoundTriggers and ModulationSources are registered in RegisterSoundScriptCommon()

    // class SoundScriptTemplate
    SoundScriptTemplate::RegisterRefCountingObject(engine, "SoundScriptTemplateClass");
    SoundScriptTemplatePtr::RegisterRefCountingObjectPtr(engine, "SoundScriptTemplateClassPtr", "SoundScriptTemplateClass");

    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "int getNumSounds()", asMETHOD(SoundScriptTemplate, getNumSounds), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "string getSoundName(int)", asMETHOD(SoundScriptTemplate, getSoundName), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "float getSoundPitch(int)", asMETHOD(SoundScriptTemplate, getSoundPitch), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "string getStartSoundName()", asMETHOD(SoundScriptTemplate, getStartSoundName), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "float getStartSoundPitch()", asMETHOD(SoundScriptTemplate, getStartSoundPitch), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "string getStopSoundName()", asMETHOD(SoundScriptTemplate, getStopSoundName), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "float getStopSoundPitch()", asMETHOD(SoundScriptTemplate, getStopSoundPitch), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "string getName()", asMETHOD(SoundScriptTemplate, getName), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "string getFileName()", asMETHOD(SoundScriptTemplate, getFileName), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "string getGroupName()", asMETHOD(SoundScriptTemplate, getGroupName), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptTemplateClass", "bool isBaseTemplate()", asMETHOD(SoundScriptTemplate, isBaseTemplate), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    // class Sound
    Sound::RegisterRefCountingObject(engine, "SoundClass");
    SoundPtr::RegisterRefCountingObjectPtr(engine, "SoundClassPtr", "SoundClass");

    result = engine->RegisterObjectMethod("SoundClass", "void setPitch(float pitch)", asMETHOD(Sound, setPitch), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "void setGain(float gain)", asMETHOD(Sound, setGain), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "void setPosition(vector3 pos)", asMETHOD(Sound, setPosition), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "void setVelocity(vector3 vel)", asMETHOD(Sound, setVelocity), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "void setLoop(bool loop)", asMETHOD(Sound, setLoop), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "void setEnabled(bool e)", asMETHOD(Sound, setEnabled), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "void play()", asMETHOD(Sound, play), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "void stop()", asMETHOD(Sound, stop), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("SoundClass", "bool getEnabled()", asMETHOD(Sound, getEnabled), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "bool isPlaying()", asMETHOD(Sound, isPlaying), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "float getAudibility() ", asMETHOD(Sound, getAudibility), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "float getGain() ", asMETHOD(Sound, getGain), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "float getPitch() ", asMETHOD(Sound, getPitch), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "bool getLoop()", asMETHOD(Sound, getLoop), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "int getCurrentHardwareIndex()", asMETHOD(Sound, getCurrentHardwareIndex), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "uint getBuffer()", asMETHOD(Sound, getBuffer), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "vector3 getPosition()", asMETHOD(Sound, getPosition), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "vector3 getVelocity()", asMETHOD(Sound, getVelocity), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundClass", "int getSourceIndex()", asMETHOD(Sound, getSourceIndex), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    // class SoundScriptInstance
    SoundScriptInstance::RegisterRefCountingObject(engine, "SoundScriptInstanceClass");
    SoundScriptInstancePtr::RegisterRefCountingObjectPtr(engine, "SoundScriptInstanceClassPtr", "SoundScriptInstanceClass");

    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "void runOnce()", asMETHOD(SoundScriptInstance, runOnce), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "void setPitch(float pitch)", asMETHOD(SoundScriptInstance, setPitch), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "void setGain(float gain)", asMETHOD(SoundScriptInstance, setGain), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "void setPosition(vector3 pos)", asMETHOD(SoundScriptInstance, setPosition), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "void setVelocity(vector3 velo)", asMETHOD(SoundScriptInstance, setVelocity), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "void start()", asMETHOD(SoundScriptInstance, start), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "void stop()", asMETHOD(SoundScriptInstance, stop), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "void kill()", asMETHOD(SoundScriptInstance, kill), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "SoundScriptTemplateClassPtr@ getTemplate()", asFUNCTION(SoundScriptInstance_getTemplate), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "SoundClassPtr@ getStartSound()", asFUNCTION(SoundScriptInstance_getStartSound), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "SoundClassPtr@ getStopSound()", asFUNCTION(SoundScriptInstance_getStopSound), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "SoundClassPtr@ getSound(int pos)", asFUNCTION(SoundScriptInstance_getSound), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "float getStartSoundPitchgain()", asMETHOD(SoundScriptInstance, getStartSoundPitchgain), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "float getStopSoundPitchgain()", asMETHOD(SoundScriptInstance, getStopSoundPitchgain), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "float getSoundPitchgain(int pos)", asMETHOD(SoundScriptInstance, getSoundPitchgain), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "int getActorInstanceId()", asMETHOD(SoundScriptInstance, getActorInstanceId), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("SoundScriptInstanceClass", "const string& getInstanceName()", asMETHOD(SoundScriptInstance, getInstanceName), asCALL_THISCALL); ROR_ASSERT(result >= 0);

}
