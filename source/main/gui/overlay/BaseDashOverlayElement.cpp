/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2021 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

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

#include "BaseDashOverlayElement.h"

using namespace RoR;
using namespace Ogre;

    // Static members

BaseDashOverlayElement::CmdAnim BaseDashOverlayElement::ms_anim_cmd;
BaseDashOverlayElement::CmdLink BaseDashOverlayElement::ms_link_cmd;

    // Functions

void BaseDashOverlayElement::addCommonDashParams(Ogre::ParamDictionary* dict)
{
    dict->addParameter(ParameterDef("anim", 
        "How to animate this dashboard control."
        , PT_STRING),
        &ms_anim_cmd);

    dict->addParameter(ParameterDef("link", 
        "What gameplay data to display with this dashboard control."
        , PT_STRING),
        &ms_link_cmd);
}

