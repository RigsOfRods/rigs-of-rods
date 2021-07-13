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

#include "DashTextAreaOverlayElement.h"
#include "DashPanelOverlayElement.h"

#include <Overlay/OgreOverlayElement.h>

using namespace RoR;
using namespace Ogre;

// ----------------
// Cmd classes

String CmdAnim::doGet( const void* target ) const
{
    BaseDashOverlayElement* elem = BaseDashOverlayElement::ResolveDashElement((Ogre::OverlayElement*)target); // shamelessly dropping const
    return (elem) ? elem->getAnimStr() : "";
}
String CmdLink::doGet( const void* target ) const
{
    BaseDashOverlayElement* elem = BaseDashOverlayElement::ResolveDashElement((Ogre::OverlayElement*)target); // shamelessly dropping const
    return (elem) ? elem->getLinkStr() : "";
}

void CmdAnim::doSet( void* target, const String& val )
{
    BaseDashOverlayElement* elem = BaseDashOverlayElement::ResolveDashElement((Ogre::OverlayElement*)target);
    if (elem)
    {
        elem->setAnimStr(val);
    }
}
void CmdLink::doSet( void* target, const String& val )
{
    BaseDashOverlayElement* elem = BaseDashOverlayElement::ResolveDashElement((Ogre::OverlayElement*)target);
    if (elem)
    {
        elem->setLinkStr(val);
    }
}

// ----------------
// Base dash elem.

    // Static members

CmdAnim BaseDashOverlayElement::ms_anim_cmd;
CmdLink BaseDashOverlayElement::ms_link_cmd;

    // Functions

//static
BaseDashOverlayElement* BaseDashOverlayElement::ResolveDashElement(Ogre::OverlayElement* elem)
{
    // Since we're extending existing Ogre overlays,
    //   we can't simply cast to BaseDashOverlayElement because
    //   it doesn't derive from Ogre::OverlayElement.
    // We must cast to classes one level up in the inheritance tree
    //   which derive from both BaseDashOverlayElement and Ogre::OverlayElement

    if (elem->getTypeName() == DashTextAreaOverlayElement::OVERLAY_ELEMENT_TYPE_NAME)
    {
        return static_cast<DashTextAreaOverlayElement*>(elem);
    }
    if (elem->getTypeName() == DashPanelOverlayElement::OVERLAY_ELEMENT_TYPE_NAME)
    {
        return static_cast<DashPanelOverlayElement*>(elem);
    }
    else
    {
        return nullptr;
    }
}

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

