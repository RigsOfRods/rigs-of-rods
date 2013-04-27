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
#ifndef __GlobalEnvironment_H_
#define __GlobalEnvironment_H_

class CameraManager;
class Character;
class Collisions;
class IHeightFinder;
class SurveyMapManager;
class Network;
class RoRFrameListener;
class SkyManager;
class TerrainManager;
class Water;

class GlobalEnvironment
{
public:

	GlobalEnvironment() :
		  cameraManager(0)
		, embeddedMode(false)
		, frameListener(0)
		, mainCamera(0)
		, network(0)
		, ogreRoot(0)
		, player(0)
		, renderWindow(0)
		, sceneManager(0)
		, sky(0)
		, surveyMap(0)
		, terrainManager(0)
		, viewPort(0)
	    , collisions(0)
	{
	}

	Ogre::Camera *mainCamera;
	Ogre::RenderWindow *renderWindow;
	Ogre::Root *ogreRoot;
	Ogre::SceneManager *sceneManager;
	Ogre::Viewport *viewPort;

	CameraManager *cameraManager;
	Character *player;
	Collisions *collisions;
	SurveyMapManager *surveyMap;
	Network *network;
	RoRFrameListener *frameListener;
	SkyManager *sky;
	TerrainManager *terrainManager;

	bool embeddedMode;
};

#endif // __GlobalEnvironment_H_
