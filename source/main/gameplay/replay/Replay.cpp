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
#include "Replay.h"

#include <Ogre.h>

#include "Utils.h"

#ifdef USE_MYGUI
#include "GUIManager.h"
#endif // MYGUI

#include "Language.h"

using namespace Ogre;

Replay::Replay(Beam *b, int _numFrames)
{
	numNodes = b->getNodeCount();
	numBeams = b->getBeamCount();
	numFrames = _numFrames;

	replayTimer = new Timer();

	// DO NOT get memory here, get memory when we use it first time!
	nodes = 0;
	beams = 0;
	times = 0;

	outOfMemory = false;

	unsigned long bsize = (numNodes * numFrames * sizeof(node_simple_t) + numBeams * numFrames * sizeof(beam_simple_t) + numFrames * sizeof(unsigned long)) / 1024.0f;
	LOG("replay buffer size: " + TOSTRING(bsize) + " kB");

	writeIndex = 0;
	firstRun = 1;

#ifdef USE_MYGUI
	// windowing
	int width = 300;
	int height = 60;
	int x = (MyGUI::RenderManager::getInstance().getViewSize().width - width) / 2;
	int y = 0;

	panel = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>("Panel", x, y, width, height, MyGUI::Align::HCenter | MyGUI::Align::Top, "Back");
	//panel->setCaption(_L("Replay"));
	panel->setAlpha(0.6);

	pr = panel->createWidget<MyGUI::Progress>("Progress", 10, 10, 280, 20,  MyGUI::Align::Default);
	pr->setProgressRange(_numFrames);
	pr->setProgressPosition(0);


	txt = panel->createWidget<MyGUI::StaticText>("StaticText", 10, 30, 280, 20,  MyGUI::Align::Default);
	txt->setCaption(_L("Position:"));

	panel->setVisible(false);
#endif // MYGUI
}

Replay::~Replay()
{
	if (nodes)
	{
		free(nodes); nodes=0;
		free(beams); beams=0;
		free(times); times=0;
	}
	delete replayTimer;
}

void *Replay::getWriteBuffer(int type)
{
	if (outOfMemory) return 0;
	if (!nodes)
	{
		// get memory
		nodes = (node_simple_t*)calloc(numNodes * numFrames, sizeof(node_simple_t));
		if (!nodes)
		{
			outOfMemory=true;
			return 0;
		}
		beams = (beam_simple_t*)calloc(numBeams * numFrames, sizeof(beam_simple_t));	
		if (!beams)
		{
			free(nodes); nodes=0;
			outOfMemory=true;
			return 0;
		}
		times = (unsigned long*)calloc(numFrames, sizeof(unsigned long));
		if (!times)
		{
			free(nodes); nodes=0;
			free(beams); beams=0;
			outOfMemory=true;
			return 0;
		}
	}
	void *ptr = 0;
	times[writeIndex] = replayTimer->getMicroseconds();
	if (type == 0)
	{
		// nodes
		ptr = (void *)(nodes + (writeIndex * numNodes));
	}else if (type == 1)
	{
		// beams
		ptr = (void *)(beams + (writeIndex * numBeams));
	}
	return ptr;
}

void Replay::writeDone()
{
	if (outOfMemory) return;
	writeIndex++;
	if (writeIndex == numFrames)
	{
		firstRun = 0;
		writeIndex = 0;
	}
}

//we take negative offsets only
void *Replay::getReadBuffer(int offset, int type, unsigned long &time)
{
	if (offset >= 0) offset=-1;
	if (offset <= -numFrames) offset = -numFrames + 1;
	
	int delta = writeIndex + offset;
	if (delta < 0)
		if (firstRun && type == 0)
			return (void *)(nodes);
		else if (firstRun && type == 1)
			return (void *)(beams);
		else
			delta += numFrames;
	
	// set the time
	time = times[delta];
	curFrameTime = time;
	curOffset = offset;
	updateGUI();
	
	if (outOfMemory) return 0;

	// return buffer pointer
	if (type == 0)
		return (void *)(nodes + delta * numNodes);
	else if (type == 1)
		return (void *)(beams + delta * numBeams);
	return 0;
}

void Replay::updateGUI()
{
#ifdef USE_MYGUI
	if (outOfMemory)
	{
		txt->setCaption(_L("Out of Memory"));
		pr->setProgressPosition(0);
	} else
	{
		wchar_t tmp[128] = L"";
		unsigned long t = curFrameTime;
		UTFString format = _L("Position: %0.6f s, frame %i / %i");
		swprintf(tmp, 128, format.asWStr_c_str(), ((float)t)/1000000.0f, curOffset, numFrames);
		txt->setCaption(convertToMyGUIString(tmp, 128));
		pr->setProgressPosition(abs(curOffset));
	}
#endif // MYGUI
}

unsigned long Replay::getLastReadTime()
{
	return curFrameTime;
}

void Replay::setVisible(bool value)
{
#ifdef USE_MYGUI
	panel->setVisible(value);
	// we need no mouse yet
	//MyGUI::PointerManager::getInstance().setVisible(value);
#endif //MYGUI
}

bool Replay::getVisible()
{
#ifdef USE_MYGUI
	return panel->getVisible();
#else
	return false;
#endif //MYGUI
}


