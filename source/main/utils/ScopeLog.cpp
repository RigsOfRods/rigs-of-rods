/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ScopeLog.h"
#include "Ogre.h"
#include "Settings.h"

#include <stdio.h> // for remove
#include <time.h>
#include "RoRVersion.h"
#include "Utils.h"

// created by thomas fischer thomas{AT}thomasfischer{DOT}biz, 18th of Juli 2009

using namespace Ogre;

ScopeLog::ScopeLog(String name) : orgLog(0), logFileName(), name(name), f(0), headerWritten(false), counter(0), disabled(false), warning(0), error(0), obsoleteWarning(0), info(0)
{
	if (!BSETTING("Advanced Logging", false))
	{
		disabled=true;
		return;
	}
	// get original log file
	orgLog = LogManager::getSingleton().getDefaultLog();

	// determine a filename
	name = stripNonASCII(name);
	logFileName = SSETTING("Log Path", "") + SSETTING("dirsep", "\\") + "_" + name + ".html";
	LOG("creating scopelog: " + logFileName);
	
	// create a new log file
	f = fopen(logFileName.c_str(), "w");

	// add self as listener
	orgLog->addListener(this);
}

ScopeLog::~ScopeLog()
{
	if (disabled) return;
	// remove self as listener
	orgLog->removeListener(this);

	// destroy our log
	if (f)
	{
		time_t t = time(NULL);
		fprintf(f, "<tr><td colspan=\"4\" class='logtd'>Log ended: %s</td></tr>\n", ctime(&t));
		fprintf(f, "</body></html>");
		fclose(f);
	}

	// if the new log file is empty, remove it.
	if (!headerWritten)
	{
		remove(logFileName.c_str());
	}
}

	// method from Ogre::LogListener
#if OGRE_VERSION < ((1 << 16) | (8 << 8 ) | 0)
void ScopeLog::messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName)
#else
void ScopeLog::messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName, bool& skipThisMessage)
#endif // OGRE_VERSION
{
	if (!f) return;
	
	counter++;
	if (!headerWritten)
	{
		time_t t = time(NULL);
		fprintf(f, "<html><header><title>%s</title>\n", name.c_str());
		fprintf(f, "<style type=\"text/css\">\n");
		fprintf(f, ".LogMessageLevel1  { font-size:0.9em;color:DarkGreen; }\n");
		fprintf(f, ".LogMessageLevel2  { font-size:1.0em;color:Black; }\n");
		fprintf(f, ".LogMessageLevel3  { font-size:1.2em;color:DarkRed; }\n");
		fprintf(f, ".Warning           { font-size:1.1em;color:Orange; }\n");
		fprintf(f, ".WarningMeshFormat { font-size:0.8em;color:GoldenRod; }\n");
		fprintf(f, ".OgreNotice        { font-size:0.8em;color:OliveDrab; }\n");
		fprintf(f, ".RoRNotice         { font-size:0.8em;color:SeaGreen; }\n");
		fprintf(f, ".CompilerError     { font-size:1.2em;color:Red; }\n");
		fprintf(f, ".MaterialError     { font-size:1.2em;color:Red; }\n");
		fprintf(f, ".GeneralError      { font-size:1.2em;color:Red; }\n");
		fprintf(f, ".IgnoreThis        { font-size:0.8em;color:DarkGrey; }\n");
		fprintf(f, ".BeamInputOutputInfo    { font-size:0.9em;color:DeepSkyBlue; }\n");
		fprintf(f, ".BeamInputOutputWarning { font-size:1.2em;color:Orange; }\n");
		fprintf(f, ".BeamInputOutputError   { font-size:1.2em;color:OrangeRed; }\n");
		fprintf(f, ".BeamInputOutputFatal   { font-size:1.2em;color:DarkRed; }\n");
		fprintf(f, ".tableheader       { font-weight:bold; }\n");
		fprintf(f, ".logtable          { border-collapse:collapse;font-family:monospace;border:1px solid #aaaaaa; }\n");
		fprintf(f, ".logtd             { border:1px solid #aaaaaa;vertical-align:top; }\n");
		fprintf(f, "</style>\n");
		fprintf(f, "</header><body>\n");
		fprintf(f, "Log for <b>%s</b> created on <b>%s</b> with <b>Rigs of Rods %s</b> ", name.c_str(), ctime(&t), ROR_VERSION_STRING);
		fprintf(f, "(built at %s on %s )<br/>\n", __DATE__, __TIME__);

		fprintf(f, "<table class=\"logtable\"><tr class='tableheader'><td class='logtd'>counter</td><td class='logtd'>ms since start</td><td class='logtd'>type</td><td class='logtd'>message</td></tr>\n");
		fprintf(f, "<tr><td colspan=\"4\" class='logtd'>Log started: %s</td></tr>\n", ctime(&t));
		headerWritten = true;
	}

	// use type depending on log message level
	char type[50]="";
	sprintf(type, "LogMessageLevel%d", lml);

	// reminder: this if switch is highly sorted
	if (message.find("you should upgrade it as soon as possible using the OgreMeshUpgrade tool") != String::npos)
	{
		sprintf(type, "WarningMeshFormat");
		obsoleteWarning++;
	}
	else if (message.find("WARNING:") != String::npos)
	{
		sprintf(type, "Warning");
		warning++;
	}
	else if (message.find("Can't assign material ") != String::npos)
	{
		sprintf(type, "MaterialError");
		error++;
	}
	else if (message.find("Compiler error: ") != String::npos)
	{
		sprintf(type, "CompilerError");
		error++;
	}
	else if (message.find("Invalid WAV file: ") != String::npos)
	{
		sprintf(type, "GeneralError");
		error++;
	}
	else if (message.find("Error while loading Terrain: ") != String::npos)
	{
		sprintf(type, "GeneralError");
		error++;
	}
	else if (message.find("Error loading texture ") != String::npos)
	{
		sprintf(type, "GeneralError");
		error++;
	}
	else if (message.find("ODEF: ") != String::npos)
	{
		sprintf(type, "BeamInputOutputInfo");
		info++;
	}
	else if (message.find("BIO|INFO") != String::npos)
	{
		sprintf(type, "BeamInputOutputInfo");
		info++;
	}
	else if (message.find("BIO|WARNING") != String::npos)
	{
		sprintf(type, "BeamInputOutputWarning");
		// no counter usage, handle differently for BIO
	}
	else if (message.find("BIO|ERROR") != String::npos)
	{
		sprintf(type, "BeamInputOutputError");
		// no counter usage, handle differently for BIO
	}
	else if (message.find("BIO|FATAL") != String::npos)
	{
		sprintf(type, "BeamInputOutputFatal");
		// no counter usage, handle differently for BIO
	}
	else if (message.find("Inertia|") != String::npos)
	{
		sprintf(type, "BeamInputOutputInfo");
		info++;
	}
	else if (message.find("Mesh: Loading ") != String::npos)
	{
		sprintf(type, "OgreNotice");
		info++;
	}
	else if (message.find("Loading 2D Texture") != String::npos)
	{
		sprintf(type, "OgreNotice");
		info++;
	}
	else if (message.find("Loading 2D Texture") != String::npos)
	{
		sprintf(type, "OgreNotice");
		info++;
	}
	else if (message.find("Texture: ") != String::npos)
	{
		sprintf(type, "OgreNotice");
		info++;
	}
	else if (message.find("Caelum: ") != String::npos)
	{
		sprintf(type, "OgreNotice");
		info++;
	}
	else if (message.find("Info: Freetype returned ") != String::npos)
	{
		sprintf(type, "IgnoreThis");
		// no counter
	}
	else if (message.find("static map icon not found: ") != String::npos)
	{
		sprintf(type, "IgnoreThis");
		// no counter
	}
	else if (message.find("COLL: ") != String::npos)
	{
		sprintf(type, "RoRNotice");
		info++;
	}
	else if (message.find("Loading WAV file ") != String::npos)
	{
		sprintf(type, "RoRNotice");
		info++;
	}
	else if (message.find("SoundScriptInstance: instance created: ") != String::npos)
	{
		sprintf(type, "RoRNotice");
		info++;
	}
	else if (message.find("FLEXBODY ") != String::npos)
	{
		sprintf(type, "RoRNotice");
		info++;
	}
	else if (message.find("MaterialFunctionMapper: replaced mesh material ") != String::npos)
	{
		sprintf(type, "RoRNotice");
		info++;
	}
	else if (message.find("MaterialFunctionMapper: replaced entity material ") != String::npos)
	{
		sprintf(type, "RoRNotice");
		info++;
	}

	

	unsigned long time = Ogre::Root::getSingleton().getTimer()->getMilliseconds();
	fprintf(f, "<tr class='%s'>"\
		"<td class='logtd'><a name='%d' href='#%d'>%d</a></td>" \
		"<td class='logtd'>%ld</td>" \
		"<td class='logtd'>%s</td>" \
		"<td class='logtd'>%s</td>" \
		"</tr>\n", type, counter, counter, counter, time, type, message.c_str());
}
