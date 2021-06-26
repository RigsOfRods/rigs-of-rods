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

// Since we're extending existing Ogre overlays,
//  we can't simply cast to BaseDashOverlayElement because
//  it doesn't directly derive from OverlayElement and it's bases.
// To correctly propagate the values, we must cast to classes
//  one level up in the inheritance tree.

String CmdAnim::doGet( const void* target ) const
{
    const Ogre::OverlayElement* elem = static_cast<const Ogre::OverlayElement*>(target);
    if (elem->getTypeName() == DashTextAreaOverlayElement::OVERLAY_ELEMENT_TYPE_NAME)
    {
        return static_cast<const DashTextAreaOverlayElement*>(target)->getAnimStr();
    }
    if (elem->getTypeName() == DashPanelOverlayElement::OVERLAY_ELEMENT_TYPE_NAME)
    {
        return static_cast<const DashPanelOverlayElement*>(target)->getAnimStr();
    }
    else
    {
        return "";
    }
}
String CmdLink::doGet( const void* target ) const
{
    const Ogre::OverlayElement* elem = static_cast<const Ogre::OverlayElement*>(target);
    if (elem->getTypeName() == DashTextAreaOverlayElement::OVERLAY_ELEMENT_TYPE_NAME)
    {
        return static_cast<const DashTextAreaOverlayElement*>(target)->getLinkStr();
    }
    if (elem->getTypeName() == DashPanelOverlayElement::OVERLAY_ELEMENT_TYPE_NAME)
    {
        return static_cast<const DashPanelOverlayElement*>(target)->getLinkStr();
    }
    else
    {
        return "";
    }
}

void CmdAnim::doSet( void* target, const String& val )
{
    const Ogre::OverlayElement* elem = static_cast<const Ogre::OverlayElement*>(target);
    if (elem->getTypeName() == DashTextAreaOverlayElement::OVERLAY_ELEMENT_TYPE_NAME)
    {
        static_cast< DashTextAreaOverlayElement*>(target)->setAnimStr(val);
    }
    if (elem->getTypeName() == DashPanelOverlayElement::OVERLAY_ELEMENT_TYPE_NAME)
    {
        static_cast< DashPanelOverlayElement*>(target)->setAnimStr(val);
    }
}
void CmdLink::doSet( void* target, const String& val )
{
    const Ogre::OverlayElement* elem = static_cast<const Ogre::OverlayElement*>(target);
    if (elem->getTypeName() == DashTextAreaOverlayElement::OVERLAY_ELEMENT_TYPE_NAME)
    {
        static_cast< DashTextAreaOverlayElement*>(target)->setLinkStr(val);
    }
    if (elem->getTypeName() == DashPanelOverlayElement::OVERLAY_ELEMENT_TYPE_NAME)
    {
        static_cast< DashPanelOverlayElement*>(target)->setLinkStr(val);
    }
}

// ----------------
// Base dash elem.

    // Static members

CmdAnim BaseDashOverlayElement::ms_anim_cmd;
CmdLink BaseDashOverlayElement::ms_link_cmd;

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

