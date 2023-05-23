/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

/// @file 
/// @brief   This creates a billboarding object that displays a text.
/// @author  2003 by cTh see gavocanov@rambler.ru
/// @update  2006 by barraq see nospam@barraquand.com

#pragma once

#include "Application.h"

#include <Ogre.h>
#include <Overlay/OgreFontManager.h>

namespace RoR {

/// @addtogroup Gfx
/// @{

class MovableText : public Ogre::MovableObject, public Ogre::Renderable
{
    /******************************** MovableText data ****************************/
public:
    enum HorizontalAlignment    {H_LEFT, H_CENTER};
    enum VerticalAlignment      {V_BELOW, V_ABOVE};

protected:
    std::string			mFontName;
    std::string			mType;
    Ogre::String			    mName;
    std::string			mCaption;
    HorizontalAlignment	mHorizontalAlignment;
    VerticalAlignment	mVerticalAlignment;

    Ogre::ColourValue		mColor;
    Ogre::RenderOperation	mRenderOp;
    Ogre::AxisAlignedBox	mAABB;
    Ogre::LightList		mLList;

    Ogre::Real			mCharHeight;
    Ogre::Real			mSpaceWidth;

    bool			mNeedUpdate;
    bool			mUpdateColors;
    bool			mOnTop;

    Ogre::Real			mTimeUntilNextToggle;
    Ogre::Real			mRadius;
    Ogre::Real            mAdditionalHeight;

    Ogre::Camera			*mpCam;
    Ogre::RenderWindow	*mpWin;
    Ogre::Font			*mpFont;
    Ogre::MaterialPtr		mpMaterial;
    Ogre::MaterialPtr		mpBackgroundMaterial;

    /******************************** public methods ******************************/
public:
    MovableText(const std::string &name, const std::string &caption, 
                const std::string &fontName = "highcontrast_black",
                Ogre::Real charHeight = 1.0, const Ogre::ColourValue &color = Ogre::ColourValue::Black);
    virtual ~MovableText();

    // Add to build on Shoggoth:
    virtual void visitRenderables(Ogre::Renderable::Visitor* visitor, bool debugRenderables = false) {};

    // Set settings
    void    setFontName(const std::string &fontName);
    void    setCaption(const std::string &caption);
    void    setColor(const Ogre::ColourValue &color);
    void    setCharacterHeight(Ogre::Real height);
    void    setSpaceWidth(Ogre::Real width);
    void    setTextAlignment(const HorizontalAlignment& horizontalAlignment, const VerticalAlignment& verticalAlignment);
    void    setAdditionalHeight(Ogre::Real height );
    void    showOnTop(bool show=true);

    // Get settings
    const   std::string          &getFontName() const {return mFontName;}
    const   std::string          &getCaption() const {return mCaption;}
    const   Ogre::ColourValue     &getColor() const {return mColor;}

    Ogre::uint    getCharacterHeight() const {return (Ogre::uint) mCharHeight;}
    Ogre::uint    getSpaceWidth() const {return (Ogre::uint) mSpaceWidth;}
    Ogre::Real    getAdditionalHeight() const {return mAdditionalHeight;}
    bool    getShowOnTop() const {return mOnTop;}
    Ogre::AxisAlignedBox	        GetAABB(void) { return mAABB; }

    /******************************** protected methods and overload **************/
protected:

    // from MovableText, create the object
    void	_setupGeometry();
    void	_updateColors();

    // from MovableObject
    void    getWorldTransforms(Ogre::Matrix4 *xform) const;
    Ogre::Real    getBoundingRadius(void) const {return mRadius;};
    Ogre::Real    getSquaredViewDepth(const Ogre::Camera *cam) const {return 0;};
    const   Ogre::Quaternion        &getWorldOrientation(void) const;
    const   Ogre::Vector3           &getWorldPosition(void) const;
    const   Ogre::AxisAlignedBox    &getBoundingBox(void) const {return mAABB;};
    const   Ogre::String            &getName(void) const {return mName;};
    const   Ogre::String            &getMovableType(void) const {static Ogre::String movType = "MovableText"; return movType;};

    void    _notifyCurrentCamera(Ogre::Camera *cam);
    void    _updateRenderQueue(Ogre::RenderQueue* queue);

    // from renderable
    void    getRenderOperation(Ogre::RenderOperation &op);
    const   Ogre::MaterialPtr       &getMaterial(void) const {ROR_ASSERT(!mpMaterial.isNull());return mpMaterial;};
    const   Ogre::LightList         &getLights(void) const {return mLList;};
};

/// @} // addtogroup Gfx

} // namespace

