/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2015 Petr Ohlidal

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
	@author Petr Ohlidal
	@date   04/2015
*/

#include "RigDef_Node.h"

#include "RoRPrerequisites.h"

using namespace RigDef;

// Ctors

Node::Id::Id():
    m_id_num(0),
    m_flags(0)
{}

Node::Id::Id(unsigned int num):
    m_id_num(num),
    m_flags(0)
{
    this->SetNum(num);    
}

Node::Id::Id(std::string const & id_str):
    m_id_num(0),
    m_flags(0)
{
    this->SetStr(id_str);
}

// Setters

void Node::Id::SetNum(unsigned int num)
{
    m_id_num = num;
    m_id_str = TOSTRING(num);
    BITMASK_SET_0(m_flags, IS_TYPE_NAMED);
    BITMASK_SET_1(m_flags, IS_TYPE_NUMBERED | IS_VALID);
}

void Node::Id::SetStr(std::string const & id_str)
{
    m_id_num = 0;
    m_id_str = id_str;
    BITMASK_SET_0(m_flags, IS_TYPE_NUMBERED);
    BITMASK_SET_1(m_flags, IS_TYPE_NAMED | IS_VALID);
}

// Util
void Node::Id::Invalidate()
{
    m_id_num = 0;
    m_id_str.clear();
    m_flags = 0;
}

Node::Ref::Ref(std::string const & id_str, unsigned int id_num, unsigned flags):
    m_flags(0)
{
    m_id = id_str;
    m_id_as_number = id_num;
    BITMASK_SET_1(m_flags, flags);
}

Node::Ref::Ref():
    m_flags(0),
    m_id_as_number(0)
{
}

void Node::Ref::Invalidate()
{
    m_id_as_number = 0;
    m_id.clear();
    m_flags = 0;
}

std::string Node::Ref::ToString() const
{
    std::stringstream msg;
    msg << "Node::Ref(id:"<<m_id<<",flags:[";
    if(GetImportState_IsValid           ()) { msg << " IMPORT_STATE_IS_VALID"            ; }
    if(GetImportState_MustCheckNamedFirst        ()) { msg << " IMPORT_STATE_MUST_CHECK_NAMED_FIRST"        ; }
    if(GetImportState_IsResolvedNamed   ()) { msg << " IMPORT_STATE_IS_RESOLVED_NAMED"   ; }
    if(GetImportState_IsResolvedNumbered()) { msg << " IMPORT_STATE_IS_RESOLVED_NUMBERED"; }                                  
    if(GetRegularState_IsValid          ()) { msg << " REGULAR_STATE_IS_VALID"           ; }
    if(GetRegularState_IsNamed          ()) { msg << " REGULAR_STATE_IS_NAMED"           ; }
    if(GetRegularState_IsNumbered       ()) { msg << " REGULAR_STATE_IS_NUMBERED"        ; }
    msg << "])";
    return msg.str();
}

std::string Node::Id::ToString() const
{
    return std::string("Node::Id(") + this->Str() + (this->IsTypeNumbered() ? " [NUMBERED])" : " [NAMED])");
}
