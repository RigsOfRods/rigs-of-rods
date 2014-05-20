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
#ifndef __RoRFrameListener_H_
#define __RoRFrameListener_H_

#include "RoRPrerequisites.h"

#include "BeamData.h"
#include "Ogre.h"

#include <pthread.h>

// Forward declarations
class Character;
class Envmap;
class ForceFeedback;

namespace MOC
{
	class CollisionTools;
}

namespace Ogre
{
	class TerrainGroup;
}


class RoRFrameListener: public Ogre::FrameListener, public Ogre::WindowEventListener, public ZeroedMemoryAllocator
{
public:
	// Constructor takes a RenderWindow because it uses that to determine input context
	RoRFrameListener(AppState *parent, Ogre::String inputhwnd=0);
	virtual ~RoRFrameListener();

	ChatSystem *netChat;

	bool freeTruckPosition; // Used for initial truck loading

	float netcheckGUITimer;

	int loading_state;

	enum LoadingStatuses { NONE_LOADED, TERRAIN_LOADED, ALL_LOADED, EXITING, EDITING, RELOADING, PAUSE, TERRAIN_EDITOR };
	
	Ogre::Vector3 reload_pos;

protected:

#ifdef USE_MPLATFORM
	MPlatform_Base *mplatform;
#endif //USE_MPLATFORM

#ifdef USE_OIS_G27
	OIS::Win32LogitechLEDs *leds;
#endif //USE_OIS_G27

	AppState *parentState;
	Dashboard *dashboard;
	DOFManager *dof;
	ForceFeedback *forcefeedback;
	HeatHaze *heathaze;

	Ogre::Quaternion reload_dir;
	Ogre::Real distgrabbed;
	Ogre::Real mTimeUntilNextToggle; // just to stop toggles flipping too fast
	Ogre::SceneNode *dirArrowNode;
	Ogre::SceneNode *pointerDestination;
	Ogre::String inputhwnd;
	Ogre::String terrainUID;
	Ogre::Vector3 dirArrowPointed;
	Ogre::Vector3 persostart;

	OverlayWrapper *ow;
	bool benchmarking;
	bool chatlock;
	bool dirvisible;
	bool enablePosStor;
	bool flipflop;
	bool hidegui;
	bool initialized;
	bool mTruckInfoOn;
	bool pressure_pressed;

	char screenshotformat[256];
	char terrainmap[1024];
	
	collision_box_t *reload_box;
	double rtime;

	float clutch;
	float mouseGrabForce;
	float terrainxsize;
	float terrainzsize;
	//float truckx, trucky, truckz;

	int flaresMode;
	int gameStartTime;
	int inputGrabMode;
	int joyshiftlock;
	int mStatsOn;
	int mouseGrabState;
	int netPointToUID;
	int nodegrabbed;
	int objectCounter;
	int raceStartTime;
	int screenHeight;
	int screenWidth;
	int shaderSchemeMode;
	int truckgrabbed;
	


	unsigned int mNumScreenShots;
	
	bool updateTruckMirrors(float dt);

	int setupBenchmark();

	void gridScreenshots(Ogre::RenderWindow* pRenderWindow, Ogre::Camera* pCamera, const int& pGridSize, const Ogre::String& path, const Ogre::String& pFileName, const Ogre::String& pFileExtention, const bool& pStitchGridImages);

	void initSoftShadows();
	void initializeCompontents();
	void updateGUI(float dt); // update engine panel
	void updateIO(float dt);
	void updateStats(void);

	// WindowEventListener
	void windowMoved(Ogre::RenderWindow* rw);
	void windowClosed(Ogre::RenderWindow* rw);
	void windowFocusChange(Ogre::RenderWindow* rw);

public: // public methods

	OverlayWrapper *getOverlayWrapper() { return ow; };

	bool RTSSgenerateShadersForMaterial(Ogre::String curMaterialName, Ogre::String normalTextureName);
	bool frameEnded(const Ogre::FrameEvent& evt);
	bool frameStarted(const Ogre::FrameEvent& evt); // Override frameStarted event to process that (don't care about frameEnded)

	bool updateEvents(float dt);
	double getTime() { return rtime; };
	float stopTimer();

	int getLoadingState() { return loading_state; };
	int getNetPointToUID() { return netPointToUID; };

	void changedCurrentTruck(Beam *previousTruck, Beam *currentTruck);
	void checkRemoteStreamResultsChanged();
	void hideGUI(bool visible);
	void hideMap();
	void initTrucks(bool loadmanual, Ogre::String selected, Ogre::String selectedExtension = "", const std::vector<Ogre::String> *truckconfig = 0, bool enterTruck = false, Skin *skin = NULL);
	
	void loadTerrain(Ogre::String terrainfile);

	void checkSpeedlimit(Beam* curr_truck, float dt);
	void netDisconnectTruck(int number);
	void pauseSim(bool value);
	void reloadCurrentTruck();
	void removeBeam(Beam *);
	void RTSSgenerateShaders(Ogre::Entity *entity, Ogre::String normalTextureName);
	void setDirectionArrow(char *text, Ogre::Vector3 position);
	void setLoadingState(int value);
	void setNetPointToUID(int uid);
	void showLoad(int type, const Ogre::String &instance, const Ogre::String &box);
	void showspray(bool s);
	void shutdown_final();
	void startTimer();
	void updateCruiseControl(Beam* curr_truck, float dt);
	void updateRacingGUI();
	void windowResized(Ogre::RenderWindow* rw); // this needs to be public so we can call it manually in embedded mode
};

#endif // __RoRFrameListener_H_
