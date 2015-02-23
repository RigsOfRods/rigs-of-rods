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
#ifndef __SkidMark_H_
#define __SkidMark_H_

#include "RoRPrerequisites.h"

#include "Singleton.h"

class SkidmarkManager : public RoRSingleton<SkidmarkManager>, public ZeroedMemoryAllocator
{
public:

	SkidmarkManager();
	~SkidmarkManager();
	
	int getTexture(Ogre::String model, Ogre::String ground, float slip, Ogre::String &texture);

private:

	typedef struct _skidmark_config
	{
		Ogre::String ground;
		Ogre::String texture;
		float slipFrom;
		float slipTo;
	} skidmark_config_t;

	int loadDefaultModels();
	std::map <Ogre::String, std::vector<skidmark_config_t> > models;
	int processLine(Ogre::StringVector args,  Ogre::String model);
};

class Skidmark : public ZeroedMemoryAllocator
{
public:

	/// Constructor - see setOperationType() for description of argument.
	Skidmark( wheel_t *wheel, Ogre::SceneNode *snode, int lenght = 500, int bucketCount = 20);
	virtual ~Skidmark();

	void updatePoint();

	void update();

private:

	static int instanceCounter;

	typedef struct _skidmark
	{
		Ogre::ManualObject *obj;
		std::vector<Ogre::Vector3> points;
		std::vector<Ogre::Real> faceSizes;
		std::vector<Ogre::String> groundTexture;
		Ogre::Vector3 lastPointAv;
		int pos;
		Ogre::ColourValue colour;
		Ogre::Vector3 face[2];
		int facecounter;
	} skidmark_t;

	Ogre::SceneNode *mNode;
	
	bool mDirty;
	float maxDistance;
	float maxDistanceSquared;
	float minDistance;
	float minDistanceSquared;
	int bucketCount;
	int lenght;
	static Ogre::Vector2 tex_coords[4];
	std::queue<skidmark_t> objects;
	wheel_t *wheel;
	
	void limitObjects();
	void addObject(Ogre::Vector3 start, Ogre::String texture);
	void setPointInt(unsigned short index, const Ogre::Vector3 &value, Ogre::Real fsize, Ogre::String texture);
	void addPoint(const Ogre::Vector3 &value, Ogre::Real fsize, Ogre::String texture);
};

#endif // __SkidMark_H_
