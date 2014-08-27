/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   RigEditor_RigFactory.cpp
	@date   06/2014
	@author Petr Ohlidal
*/

#include "RigEditor_RigFactory.h"

#include "RigDef_File.h"
#include "RigEditor_Beam.h"
#include "RigEditor_Main.h"
#include "RigEditor_Node.h"
#include "RigEditor_Rig.h"

#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgreManualObject.h>
#include <OgreSceneManager.h>
#include <OgreRenderOperation.h>

using namespace RoR;
using namespace RoR::RigEditor;

Rig* RigFactory::BuildRig(
	RigDef::File* rig_def, 
	std::vector< boost::shared_ptr<RigDef::File::Module> > & selected_modules, 
	RigEditor::Main* rig_editor
	)
{
	RigEditor::Rig* rig = new Rig();
	RigEditor::Main::Config & config = rig_editor->GetConfig();

	/* Process nodes (section "nodes") */

	for (auto module_itor = selected_modules.begin(); module_itor != selected_modules.end(); module_itor++)
	{
		for (auto node_itor = (*module_itor)->nodes.begin(); node_itor != (*module_itor)->nodes.end(); node_itor++)
		{
			bool done = false;
			RigDef::Node::Id & node_id = node_itor->id;
			while (true)
			{
				auto result = rig->m_nodes.insert( std::pair<RigDef::Node::Id, Node*>(node_id, new Node(*node_itor)) );
				if (result.second == true)
				{
					break;
				}
				else
				{
					std::stringstream msg;
					msg << "[WARNING] BuildRig(): Duplicate node ID: " << node_id.ToString();
					node_id.SetStr(node_id.ToString() + "-dup");
					msg << ", changed to: " << node_id.ToString();
					AddMessage(module_itor->get(), msg.str());
					rig->m_modified = true;
				}
			}
		}
	}

	/* Process beams (section "beams") */
	std::vector<RigDef::Beam> unlinked_beams_to_retry; // Linked to invalid nodes, will retry after other sections (esp. wheels) were processed
	// TODO: Implement the retry.

	for (auto module_itor = selected_modules.begin(); module_itor != selected_modules.end(); module_itor++)
	{
		for (auto beam_itor = (*module_itor)->beams.begin(); beam_itor != (*module_itor)->beams.end(); beam_itor++)
		{
			Node* nodes[] = {nullptr, nullptr};

			/* Find node 0 */
			auto result = rig->m_nodes.find(beam_itor->nodes[0]);
			if (result == rig->m_nodes.end())
			{
				/* Node 0 not found */
				unlinked_beams_to_retry.push_back(*beam_itor);
				std::stringstream msg;
				msg << "[Warning] BuildRig(): Beam[ " << (*beam_itor).nodes[0].ToString() << ", " << (*beam_itor).nodes[1].ToString() << "]: Node 0 not found. Will retry later.";
				AddMessage(module_itor->get(), msg.str());
			}
			else
			{
				nodes[0] = result->second; // Assign node 0

				/* Find node 1 */
				result = rig->m_nodes.find(beam_itor->nodes[1]);
				if (result == rig->m_nodes.end())
				{
					 /* Node 1 not found */
					unlinked_beams_to_retry.push_back(*beam_itor);
					std::stringstream msg;
					msg << "[Warning] BuildRig(): Beam[ " << (*beam_itor).nodes[0].ToString() << ", " << (*beam_itor).nodes[1].ToString() << "]: Node 1 not found. Will retry later.";
					AddMessage(module_itor->get(), msg.str());
				}
				else
				{
					nodes[1] = result->second; // Assing node 1
				}
			}

			rig->m_beams.push_back( new Beam(*beam_itor, nodes[0], nodes[1]) );
		}
	}

	/* Prepare material */
	if (! Ogre::MaterialManager::getSingleton().resourceExists("rig-editor-skeleton-material"))
	{
		Ogre::MaterialPtr mat = static_cast<Ogre::MaterialPtr>(
			Ogre::MaterialManager::getSingleton().create("rig-editor-skeleton-material", 
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME)
		);

		mat->getTechnique(0)->getPass(0)->createTextureUnitState();
		mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
		mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAnisotropy(3);
		mat->setLightingEnabled(false);
		mat->setReceiveShadows(false);
	}

	/* Create mesh */
	rig->m_beams_dynamic_mesh = rig_editor->GetOgreSceneManager()->createManualObject();
	rig->m_beams_dynamic_mesh->estimateIndexCount(rig->m_beams.size() * 2);
	rig->m_beams_dynamic_mesh->estimateVertexCount(rig->m_nodes.size());
	rig->m_beams_dynamic_mesh->setCastShadows(false);
	rig->m_beams_dynamic_mesh->setDynamic(true);
	rig->m_beams_dynamic_mesh->setRenderingDistance(300);

	/* Init */
	rig->m_beams_dynamic_mesh->begin("rig-editor-skeleton-material", Ogre::RenderOperation::OT_LINE_LIST);

	/* Process beams */
	for (auto itor = rig->m_beams.begin(); itor != rig->m_beams.end(); itor++)
	{
		rig->m_beams_dynamic_mesh->position((*itor)->GetNodeA()->GetPosition());
		rig->m_beams_dynamic_mesh->colour(config.beam_generic_color);
		rig->m_beams_dynamic_mesh->position((*itor)->GetNodeB()->GetPosition());
		rig->m_beams_dynamic_mesh->colour(config.beam_generic_color);
	}

	/* Finalize */
	rig->m_beams_dynamic_mesh->end();
	
	return rig;
}

void RigFactory::AddMessage(RigDef::File::Module* module, std::string const & text)
{
	std::stringstream msg;
	msg << text << " (module: " << module->name << ")";
	m_messages.push_back(msg.str());
}
