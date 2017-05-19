/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.com/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
    @file   RigEditor_Node.h
    @date   06/2014
    @author Petr Ohlidal
*/

#pragma once

#include "RigDef_Prerequisites.h"
#include "RigDef_File.h"
#include "RoRPrerequisites.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_Types.h"

namespace RoR
{

namespace RigEditor
{

class Node
{
    friend class RigEditor::Rig;

public:

    struct Flags
    {
        static const int STATE_HOVERED     = BITMASK(1);
        static const int STATE_SELECTED    = BITMASK(2);
        static const int SOURCE_CINECAM    = BITMASK(3);
    };

    Node(RigDef::Node const & def);

    Node();

    Ogre::Vector3 const & GetPosition();

    void SetPosition(Ogre::Vector3 const & pos);

    void SetScreenPosition(Vector2int const & screen_pos);

    Vector2int const & GetScreenPosition() const;

    void SetHovered(bool value);

    bool IsHovered() const;

    void SetSelected(bool value);

    bool IsSelected() const;

    bool IsSourceCineCamera() const;

    void Translate(Ogre::Vector3 const & offset);

    RigDef::Node::Id const & GetId();

    void SetDefinitionPosition(Ogre::Vector3 const & pos);

    Ogre::Vector3 const & GetDefinitionPosition();

    inline int      GetDefinitionDetacherGroup() const                { return m_definition.detacher_group; }

    inline void     SetDefinitionDetacherGroup(int detacher_group_id) { m_definition.detacher_group = detacher_group_id; }

    inline float    GetDefinitionLoadWeight() const                   { return m_definition.load_weight_override; }

    inline void     SetDefinitionLoadWeight(float load_weight)        { m_definition.load_weight_override = load_weight; }

    inline unsigned GetDefinitionFlags() const                        { return m_definition.options; }

    inline void     SetDefinitionFlags(unsigned int flags)            { m_definition.options = flags; }

protected:

    bool UnlinkBeam(Beam* beam);

    RigDef::Node           m_definition;

    Vector2int             m_screen_position;
    int                    m_flags;
    Ogre::ColourValue      m_color;
    std::list<Beam*>       m_linked_beams;
    Ogre::Vector3          m_position; ///< Current position; represents intermediate position during transformations.
};

} // namespace RigEditor

} // namespace RoR
