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
#ifndef __Character_H_
#define __Character_H_

#include "RoRPrerequisites.h"

#include "MovableText.h"
#include "Streamable.h"

class Character : public Streamable, public ZeroedMemoryAllocator
{
	friend class CharacterFactory;
	friend class Network;

public:

	Character(int source=-1, unsigned int streamid=0, int colourNumber=0, bool remote=true);
	~Character();

	Ogre::Radian getRotation() { return characterRotation; };
	Ogre::Vector3 getPosition();
	bool getPhysicsEnabled() { return physicsEnabled; };
	bool getVisible();
	int getUID() { return source; };
	
	bool isRemote() { return remote; };
	bool getBeamCoupling() { return isCoupled; };

	void setBeamCoupling(bool enabled, Beam *truck = 0);
	void setColour(int color) { this->colourNumber = color; };
	void setPhysicsEnabled(bool enabled) { physicsEnabled = enabled; };
	void setPosition(Ogre::Vector3 position);
	void setRotation(Ogre::Radian rotation);
	void setVisible(bool visible);

	void move(Ogre::Vector3 offset);

	void update(float dt);
	void updateCharacterColour();
	void updateCharacterRotation();
	void updateMapIcon();
	void updateNetLabel();

	static unsigned int characterCounter;

protected:

	void createMapEntity();

	Beam *beamCoupling;
	bool isCoupled;
	SurveyMapEntity *mapEntity;

	bool canJump;
	bool physicsEnabled;
	bool remote;
	
	Ogre::Radian characterRotation;
	Ogre::Real characterSpeed;
	Ogre::Real characterVSpeed;
	
	int colourNumber;
	int networkAuthLevel;
	int source;

	Ogre::AnimationStateSet *mAnimState;
	Ogre::Camera *mCamera;
	Ogre::MovableText *mMoveableText;
	Ogre::SceneNode *mCharacterNode;
	Ogre::String mLastAnimMode;
	Ogre::String myName;
	Ogre::UTFString networkUsername;
	Ogre::Vector3 mLastPosition;

	unsigned int myNumber;
	unsigned int streamid;
	
	void setAnimationMode(Ogre::String mode, float time=0);

	// network stuff
	typedef struct header_netdata_t
	{
		int command;
	} header_netdata_t;

	typedef struct pos_netdata_t
	{
		int command;
		float posx, posy, posz;
		float rotx, roty, rotz, rotw;
		char animationMode[256];
		float animationTime;
	} pos_netdata_t;

	typedef struct attach_netdata_t
	{
		int command;
		bool enabled;
		int source_id;
		int stream_id;
		int position;
	} attach_netdata_t;

	enum {CHARCMD_POSITION, CHARCMD_ATTACH};

	// overloaded from Streamable:
	Ogre::Timer netTimer;
	int last_net_time;

	void receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len);
	void sendStreamData();
	void sendStreamSetup();
	void setUID(int uid);

	void updateNetLabelSize();
};

#endif // __Character_H_
