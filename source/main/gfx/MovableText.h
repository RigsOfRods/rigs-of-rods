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

/// @file This creates a billboarding object that displays a text.
///
/// @author  2003 by cTh see gavocanov@rambler.ru
/// @update  2006 by barraq see nospam@barraquand.com

#pragma once

#include <Ogre.h>
#include <Overlay/OgreFontManager.h>

#include "RoRPrerequisites.h"

namespace Ogre {

class MovableText : public MovableObject, public Renderable
{
    /******************************** MovableText data ****************************/
public:
    enum HorizontalAlignment    {H_LEFT, H_CENTER};
    enum VerticalAlignment      {V_BELOW, V_ABOVE};

protected:
    UTFString			mFontName;
    UTFString			mType;
    String			    mName;
    UTFString			mCaption;
    HorizontalAlignment	mHorizontalAlignment;
    VerticalAlignment	mVerticalAlignment;

    ColourValue		mColor;
    RenderOperation	mRenderOp;
    AxisAlignedBox	mAABB;
    LightList		mLList;

    Real			mCharHeight;
    Real			mSpaceWidth;

    bool			mNeedUpdate;
    bool			mUpdateColors;
    bool			mOnTop;

    Real			mTimeUntilNextToggle;
    Real			mRadius;
    Real            mAdditionalHeight;

    Camera			*mpCam;
    RenderWindow	*mpWin;
    Font			*mpFont;
    MaterialPtr		mpMaterial;
    MaterialPtr		mpBackgroundMaterial;

    /******************************** public methods ******************************/
public:
    MovableText(const UTFString &name, const UTFString &caption, const UTFString &fontName = "CyberbitEnglish", Real charHeight = 1.0, const ColourValue &color = ColourValue::Black);
    virtual ~MovableText();

    // Add to build on Shoggoth:
#if OGRE_VERSION>0x010602
    virtual void visitRenderables(Ogre::Renderable::Visitor* visitor, bool debugRenderables = false);
#endif //OGRE_VERSION

    // Set settings
    void    setFontName(const UTFString &fontName);
    void    setCaption(const UTFString &caption);
    void    setColor(const ColourValue &color);
    void    setCharacterHeight(Real height);
    void    setSpaceWidth(Real width);
    void    setTextAlignment(const HorizontalAlignment& horizontalAlignment, const VerticalAlignment& verticalAlignment);
    void    setAdditionalHeight( Real height );
    void    showOnTop(bool show=true);

    // Get settings
    const   UTFString          &getFontName() const {return mFontName;}
    const   UTFString          &getCaption() const {return mCaption;}
    const   ColourValue     &getColor() const {return mColor;}

    uint    getCharacterHeight() const {return (uint) mCharHeight;}
    uint    getSpaceWidth() const {return (uint) mSpaceWidth;}
    Real    getAdditionalHeight() const {return mAdditionalHeight;}
    bool    getShowOnTop() const {return mOnTop;}
    AxisAlignedBox	        GetAABB(void) { return mAABB; }

    /******************************** protected methods and overload **************/
protected:

    // from MovableText, create the object
    void	_setupGeometry();
    void	_updateColors();

    // from MovableObject
    void    getWorldTransforms(Matrix4 *xform) const;
    Real    getBoundingRadius(void) const {return mRadius;};
    Real    getSquaredViewDepth(const Camera *cam) const {return 0;};
    const   Quaternion        &getWorldOrientation(void) const;
    const   Vector3           &getWorldPosition(void) const;
    const   AxisAlignedBox    &getBoundingBox(void) const {return mAABB;};
    const   String            &getName(void) const {return mName;};
    const   String            &getMovableType(void) const {static Ogre::String movType = "MovableText"; return movType;};

    void    _notifyCurrentCamera(Camera *cam);
    void    _updateRenderQueue(RenderQueue* queue);

    // from renderable
    void    getRenderOperation(RenderOperation &op);
    const   MaterialPtr       &getMaterial(void) const {assert(!mpMaterial.isNull());return mpMaterial;};
    const   LightList         &getLights(void) const {return mLList;};
};

} // namespace

