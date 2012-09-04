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
#include "MaterialFunctionMapper.h"

#include "Ogre.h"
#include "Settings.h"

using namespace Ogre;

int MaterialFunctionMapper::simpleMaterialCounter = 0;

void MaterialFunctionMapper::addMaterial(int flareid, materialmapping_t t)
{
	MaterialPtr m = Ogre::MaterialManager::getSingleton().getByName(t.material);
	if (m.isNull()) return;
	Technique *tech = m->getTechnique(0);
	if (!tech) return;
	Pass *p = tech->getPass(0);
	if (!p) return;
	// save emissive colour and then set to zero (light disabled by default)
	t.emissiveColour = p->getSelfIllumination();
	t.laststate = false;
	p->setSelfIllumination(ColourValue::ZERO);
	materialBindings[flareid].push_back(t);
}

void MaterialFunctionMapper::toggleFunction(int flareid, bool isvisible)
{
	std::vector<materialmapping_t> mb = materialBindings[flareid];
	std::vector<materialmapping_t>::iterator it;
	for (it=mb.begin(); it!=mb.end(); it++)
	{
		//if (it->laststate == isvisible)
		//	continue;
		//it->laststate = isvisible;

		MaterialPtr m = Ogre::MaterialManager::getSingleton().getByName(it->material);
		if (m.isNull()) continue;

		for (int i=0;i<m->getNumTechniques();i++)
		{
			Technique *tech = m->getTechnique(i);
			if (!tech) continue;

			if (tech->getSchemeName() == "glow")
			{
				// glowing technique
				// set the ambient value as glow amount
				Pass *p = tech->getPass(0);
				if (!p) continue;

				if (isvisible)
				{
					p->setSelfIllumination(it->emissiveColour);
					p->setAmbient(ColourValue::White);
					p->setDiffuse(ColourValue::White);
				} else
				{
					p->setSelfIllumination(ColourValue::ZERO);
					p->setAmbient(ColourValue::Black);
					p->setDiffuse(ColourValue::Black);
				}
			} else
			{
				// normal technique
				Pass *p = tech->getPass(0);
				if (!p) continue;

				TextureUnitState *tus = p->getTextureUnitState(0);
				if (!tus) continue;

				if (tus->getNumFrames() < 2)
					continue;

				int frame = isvisible?1:0;

				tus->setCurrentFrame(frame);

				if (isvisible)
					p->setSelfIllumination(it->emissiveColour);
				else
					p->setSelfIllumination(ColourValue::ZERO);
			}
		}
	}
}

void MaterialFunctionMapper::replaceMeshMaterials(Ogre::Entity *e)
{
	if (!e)
	{
		LOG("MaterialFunctionMapper: got invalid Entity in replaceMeshMaterials");
		return;
	}
	// this is not nice, but required (its not so much performance relevant ...
	for (std::map <int, std::vector<materialmapping_t> >::iterator mfb = materialBindings.begin(); mfb!=materialBindings.end(); mfb++)
	{
		for (std::vector<materialmapping_t>::iterator mm = mfb->second.begin(); mm != mfb->second.end(); mm++)
		{
			MeshPtr m = e->getMesh();
			if (!m.isNull())
			{
				for (int n=0; n < (int)m->getNumSubMeshes(); n++)
				{
					SubMesh *sm = m->getSubMesh(n);
					if (sm->getMaterialName() ==  mm->originalmaterial)
					{
						sm->setMaterialName(mm->material);
						LOG("MaterialFunctionMapper: replaced mesh material " + mm->originalmaterial + " with new new material " + mm->material + " on entity " + e->getName());
					}
				}
			}

			for (int n=0; n < (int)e->getNumSubEntities(); n++)
			{
				SubEntity *subent = e->getSubEntity(n);
				if (subent->getMaterialName() ==  mm->originalmaterial)
				{
					subent->setMaterialName(mm->material);
					LOG("MaterialFunctionMapper: replaced entity material " + mm->originalmaterial + " with new new material " + mm->material + " on entity " + e->getName());
				}
			}
		}
	}
}

void MaterialFunctionMapper::replaceSimpleMeshMaterials(Ogre::Entity *e, Ogre::ColourValue c)
{
	if (!e)
	{
		LOG("MaterialFunctionMapper: got invalid Entity in replaceSimpleMeshMaterials");
		return;
	}
	if (!BSETTING("SimpleMaterials", false)) return;

	MaterialPtr mat = MaterialManager::getSingleton().getByName("tracks/simple");
	if (mat.isNull()) return;

	String newMatName = "tracks/simple/" + TOSTRING(simpleMaterialCounter);
	MaterialPtr newmat = mat->clone(newMatName);

	newmat->getTechnique(0)->getPass(0)->setAmbient(c);

	simpleMaterialCounter++;
	
	MeshPtr m = e->getMesh();
	if (!m.isNull())
	{
		for (int n=0; n < (int)m->getNumSubMeshes(); n++)
		{
			SubMesh *sm = m->getSubMesh(n);
			sm->setMaterialName(newMatName);
		}
	}

	for (int n=0; n < (int)e->getNumSubEntities(); n++)
	{
		SubEntity *subent = e->getSubEntity(n);
		subent->setMaterialName(newMatName);
	}
}
