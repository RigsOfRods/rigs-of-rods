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
// created on 15th of May 2011 by Thomas Fischer

#include "OgreScriptBuilder.h"

#include <string>
#include <Ogre.h>

#include "SHA1.h"

using namespace std;
using namespace Ogre;

// OgreScriptBuilder
int OgreScriptBuilder::LoadScriptSection(const char *filename)
{
	// Open the script file
	string scriptFile = filename;

	DataStreamPtr ds;
	try
	{
		ds = ResourceGroupManager::getSingleton().openResource(scriptFile, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

	} catch(Ogre::Exception e)
	{
		LOG("exception upon loading script file: " + e.getFullDescription());
		return -1;
	}

	// Read the entire file
	string code;
	code.resize(ds->size());
	ds->read(&code[0], ds->size());

	// hash it
	{
		char hash_result[250];
		memset(hash_result, 0, 249);
		RoR::CSHA1 sha1;
		sha1.UpdateHash((uint8_t *)code.c_str(), (uint32_t)code.size());
		sha1.Final();
		sha1.ReportHash(hash_result, RoR::CSHA1::REPORT_HEX_SHORT);
		hash = String(hash_result);
	}

	return ProcessScriptSection(code.c_str(), filename);
}
