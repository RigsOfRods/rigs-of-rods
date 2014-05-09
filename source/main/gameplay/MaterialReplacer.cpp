#include "MaterialReplacer.h"
#include "Ogre.h"

using namespace Ogre;

void MaterialReplacer::replaceMeshMaterials(Ogre::Entity *e)
{
	MeshPtr m = e->getMesh();
	if (!m.isNull())
	{
		for (int n=0; n<(int)m->getNumSubMeshes();n++)
		{
			SubMesh *sm = m->getSubMesh(n);
			if (this->hasReplacementForMaterial(sm->getMaterialName()))
			{
				String newMat = this->getReplacementForMaterial(sm->getMaterialName());
				//LOG("Skin: replaced mesh material " + sm->getMaterialName() + " with new new material " + newMat + " on entity " + e->getName());
				sm->setMaterialName(newMat);
			}
		}
	}

	for (int n=0; n<(int)e->getNumSubEntities();n++)
	{
		SubEntity *subent = e->getSubEntity(n);
		if (this->hasReplacementForMaterial(subent->getMaterialName()))
		{
			String newMat = this->getReplacementForMaterial(subent->getMaterialName());
			//LOG("Skin: replaced mesh material " + subent->getMaterialName() + " with new new material " + newMat + " on entity " + e->getName());
			subent->setMaterialName(newMat);
		}
	}
}

int MaterialReplacer::hasReplacementForMaterial(Ogre::String material)
{
	int res = (int)replaceMaterials.count(material);
	if (!res)
		return (int)replaceMaterials.count(material);
	return res;
}

Ogre::String MaterialReplacer::getReplacementForMaterial(Ogre::String material)
{
	String res = replaceMaterials[material];
	if (res.empty())
		return replaceMaterials[material];
	return res;
}

int MaterialReplacer::addMaterialReplace(Ogre::String from, Ogre::String to)
{
	replaceMaterials[from] = to;
	return 0;
}
