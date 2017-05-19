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
    @file
    @date   06/2014
    @author Petr Ohlidal
*/

#pragma once

#include "RigDef_Prerequisites.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RoRPrerequisites.h"

namespace RoR
{

namespace RigEditor
{

class Beam
{
    friend class RigEditor::Rig;

public:

    enum Type
    {
        TYPE_PLAIN,
        TYPE_STEERING_HYDRO,   //!< Called 'hydro' in RoR jargon
        TYPE_COMMAND_HYDRO,    //!< Called 'command' in RoR jargon
        TYPE_SHOCK_ABSORBER,   //!< Called 'shock' in RoR jargon
        TYPE_SHOCK_ABSORBER_2, //!< Section 'shocks2'
        TYPE_CINECAM           //!< Section 'cinecam' - generated
    };

    Beam(void* source, Type type, RigEditor::Node* node_0, RigEditor::Node* node_1);

    ~Beam();

    // Getters
    inline Node*                      GetNodeA() const { return m_nodes[0]; }
    inline Node*                      GetNodeB() const { return m_nodes[1]; }
    inline Ogre::ColourValue const &  GetColor() const { return m_color; }
    inline Beam::Type                 GetType() const  { return m_type; }

    inline void SetColor(Ogre::ColourValue const & color) { m_color = color; }

    void DeleteDefinition();

    // Definition getters
    inline RigDef::Beam* GetDefinitionPlainBeam() const
    {
        return (GetType() == Beam::TYPE_PLAIN) ? reinterpret_cast<RigDef::Beam*>(m_source) : nullptr; 
    }

    inline RigDef::Hydro* GetDefinitionHydro() const     
    { 
        return (GetType() == Beam::TYPE_STEERING_HYDRO) ? reinterpret_cast<RigDef::Hydro*>(m_source) : nullptr; 
    }

    inline RigDef::Command2* GetDefinitionCommand2() const  
    { 
        return (GetType() == Beam::TYPE_COMMAND_HYDRO) ? reinterpret_cast<RigDef::Command2*>(m_source) : nullptr; 
    }

    inline RigDef::Shock* GetDefinitionShock() const     
    { 
        return (GetType() == Beam::TYPE_SHOCK_ABSORBER) ? reinterpret_cast<RigDef::Shock*>(m_source) : nullptr; 
    }

    inline RigDef::Shock2* GetDefinitionShock2() const     
    { 
        return (GetType() == Beam::TYPE_SHOCK_ABSORBER_2) ? reinterpret_cast<RigDef::Shock2*>(m_source) : nullptr; 
    }

protected:

    RigEditor::Node*       m_nodes[2];
    Ogre::ColourValue      m_color;
    Type                   m_type;
    void*                  m_source;
};

} // namespace RigEditor

} // namespace RoR
