/*
	Copyright (c) 2010 ASTRE Henri (http://www.visual-experiments.com)

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/

#include "CanvasContext.h"
#include <effects/SkGradientShader.h>

using namespace Ogre;
using namespace Canvas;

State::State()
{
	globalAlpha = 1.0;
	globalCompositeOperation = DrawingOperator_SourceOver;
	
	lineWidth  = 1.0;
	lineCap    = LineCap_Butt;
	lineJoin   = LineJoin_Miter;
	miterLimit = 10.0;
	lineDash   = LineDash_Solid;

	strokeColourSource = ColourSource_Uniform;
	strokeColour       = Ogre::ColourValue::Black;
	strokeGradient     = NULL;
	strokePattern      = NULL;

	fillColourSource = ColourSource_Uniform;
	fillColour       = Ogre::ColourValue::Black;
	fillGradient     = NULL;
	fillPattern      = NULL;

	shadowOffsetX = 0.0;
	shadowOffsetY = 0.0;
	shadowBlur    = 0.0;
	shadowColor   = Ogre::ColourValue::Black;

	textSize     = 10;
	textAlign    = TextAlign_Left;
	textBaseline = TextBaseline_Alphabetic;

	//String m_unparsedFont;
	//Font m_font;
	//bool m_realizedFont;
}
Context::Context(unsigned int width, unsigned int height, bool alphaEnable)
{
	mWidth       = width;
	mHeight      = height;
	mAlphaEnable = alphaEnable;

	mBitmap = new SkBitmap();
	mBitmap->setConfig(SkBitmap::kARGB_8888_Config, width, height);
	mBitmap->allocPixels();
	mBitmap->eraseARGB(0x00, 0x00, 0x00, 0x00);

	mDevice = new SkDevice(*mBitmap);
	mCanvas = new SkCanvas(mDevice);

	mFillStyle = new SkPaint;
	mFillStyle->setAntiAlias(true);

	mStrokeStyle = new SkPaint;
	mStrokeStyle->setAntiAlias(true);
	mStrokeStyle->setStyle(SkPaint::kStroke_Style);	

	mStates.push(State());
}

Context::~Context()
{
/*
	delete mDevice;
	mDevice = NULL;

	delete mBitmap;
	mBitmap = NULL;

	delete mCanvas;
	mCanvas = NULL;	
*/
}


//2D Context
void Context::save()
{	
	State state = getCurrentState();
	mStates.push(state);
	mCanvas->save();
}

void Context::restore()
{
	if (mStates.size() > 0)
		mStates.pop();
	mCanvas->restore();
}

void Context::reset()
{
	for (unsigned int i=0; i<mStates.size(); ++i)
		mStates.pop();
	//FIXME : no call to canvas->restore(); ?
	mStates.push(State());
	beginPath();
}

//Transformation
void Context::scale(float x, float y)
{
	mCanvas->scale(x, y);
}

void Context::scale(const Ogre::Vector2& scaling)
{
	scale(scaling.x, scaling.y);
}

void Context::rotate(float radian)
{
	mCanvas->rotate((SkScalar)(radian*180.0/Ogre::Math::PI));
}

void Context::rotate(const Ogre::Radian& angle)
{
	rotate(angle.valueRadians());
}

void Context::translate(float x, float y)
{
	mCanvas->translate(x, y);
}

void Context::translate(const Ogre::Vector2& translation)
{
	translate(translation.x, translation.y);
}

void Context::transform(float m11, float m12, float m21, float m22, float dx, float dy)
{
	SkMatrix matrix;
	matrix.set(0, m11); //scaleX
	matrix.set(1, m21); //skewX
	matrix.set(2, dx);  //translateX
	matrix.set(3, m12); //skewY
	matrix.set(4, m22); //scaleY
	matrix.set(5, dy);  //translateY
	matrix.set(6, 0);   //perspX
	matrix.set(7, 0);   //perspY
	matrix.set(8, 1);
	mCanvas->concat(matrix);
}

void Context::transform(const Ogre::Matrix3& transform)
{
	/*
	transform(transform[0][0], transform[0][0], 
		      transform[0][0], transform[0][0], 
			  transform[0][0], transform[0][0]);
	*/
}

void Context::setTransform(float m11, float m12, float m21, float m22, float dx, float dy)
{
	SkMatrix matrix;
	matrix.set(0, m11); //scaleX
	matrix.set(1, m21); //skewX
	matrix.set(2, dx);  //translateX
	matrix.set(3, m12); //skewY
	matrix.set(4, m22); //scaleY
	matrix.set(5, dy);  //translateY
	matrix.set(6, 0);   //perspX
	matrix.set(7, 0);   //perspY
	matrix.set(8, 1);
	mCanvas->setMatrix(matrix);
}

void Context::setTransform(const Ogre::Matrix3& transform)
{
	/*
	setTransform(transform[0][0], transform[0][0], 
		         transform[0][0], transform[0][0], 
			     transform[0][0], transform[0][0]);
	*/
}

//Image drawing
void Context::drawImage(const Ogre::Image& image, float dstX, float dstY, float dstWidth, float dstHeight)
{
	if (dstWidth == 0 || dstHeight == 0)
		return;

	Pattern* pattern = createPattern(image);

	save();
	translate(dstX, dstY);
	fillStyle(pattern);
	if (dstWidth != 0 && dstHeight != 0)
		scale(dstWidth/image.getWidth(), dstHeight/image.getHeight());
	fillRect(0.0f, 0.0f, (float) image.getWidth(), (float) image.getHeight());
	restore();

	delete pattern;
}

void Context::drawImage(const Ogre::Image& image, const Ogre::Vector2& pos, const Ogre::Vector2& dim)
{
	drawImage(image, pos.x, pos.y, dim.x, dim.y);
}

void Context::drawImage(const Ogre::Image& image, float srcX, float srcY, float srcWidth, float srcHeight, float dstX, float dstY, float dstWidth, float dstHeight)
{
	Pattern* pattern = createPattern(image);

	save();
	beginPath();
	rect(dstX, dstY, dstWidth, dstHeight);
	clip();

	Ogre::Vector2 scaling(dstWidth/srcWidth, dstHeight/srcHeight);
	scale(scaling);

	beginPath();
	fillStyle(pattern);
	translate(dstX - srcX*scaling.x, dstY - srcY*scaling.y);
	fillRect(0.0f, 0.0f, (float) image.getWidth(), (float) image.getHeight());

	getCurrentState().fillPattern = NULL;

	restore();

	delete pattern;
}

void Context::drawImage(const Ogre::Image& image, const Ogre::Vector2& srcPos, const Ogre::Vector2& srcDim, const Ogre::Vector2& dstPos, const Ogre::Vector2& dstDim)
{
	drawImage(image, srcPos.x, srcPos.y, srcDim.x, srcDim.y, dstPos.x, dstPos.y, dstDim.x, dstDim.y);
}

//Compositing
void Context::globalAlpha(float alpha)
{	
	getCurrentState().globalAlpha = alpha;
}

float Context::globalAlpha()
{
	return getCurrentState().globalAlpha;
}

void Context::globalCompositeOperation(DrawingOperator op)
{
	getCurrentState().globalCompositeOperation = op;
}

DrawingOperator Context::globalCompositeOperation()
{
	return getCurrentState().globalCompositeOperation;
}

//Line styles
void Context::lineWidth(float width)
{
	getCurrentState().lineWidth = width;	
}

float Context::lineWidth()
{
	return getCurrentState().lineWidth;
}

void Context::lineCap(LineCap lineCap)
{
	getCurrentState().lineCap = lineCap;
}

Canvas::LineCap Context::lineCap()
{
	return getCurrentState().lineCap;
}

void Context::lineJoin(LineJoin lineJoin)
{
	getCurrentState().lineJoin = lineJoin;
}

Canvas::LineJoin Context::lineJoin()
{
	return getCurrentState().lineJoin;
}

void Context::lineDash(LineDash lineDash)
{	
	getCurrentState().lineDash = lineDash;
}

Canvas::LineDash Context::lineDash()
{
	return getCurrentState().lineDash;
}

void Context::miterLimit(float limit)
{
	getCurrentState().miterLimit = limit;	
}

Ogre::Real Context::miterLimit()
{
	return getCurrentState().miterLimit;
}

//Colors, styles and shadows
void Context::strokeStyle(Ogre::ColourValue color)
{
	getCurrentState().strokeColourSource = ColourSource_Uniform;
	getCurrentState().strokeColour = color;
}

void Context::strokeStyle(Gradient* gradient)
{
	getCurrentState().strokeColourSource = ColourSource_Gradient;
	getCurrentState().strokeGradient = gradient;
}

void Context::strokeStyle(Pattern* pattern)
{
	getCurrentState().strokeColourSource = ColourSource_Pattern;
	getCurrentState().strokePattern = pattern;
}

void Context::fillStyle(Ogre::ColourValue color)
{
	getCurrentState().fillColourSource = ColourSource_Uniform;
	getCurrentState().fillColour = color;
}

void Context::fillStyle(Gradient* gradient)
{
	getCurrentState().fillColourSource = ColourSource_Gradient;
	getCurrentState().fillGradient = gradient;
}

void Context::fillStyle(Pattern* pattern)
{
	getCurrentState().fillColourSource = ColourSource_Pattern;
	getCurrentState().fillPattern = pattern;
}

void Context::shadowOffsetX(float x)
{
	getCurrentState().shadowOffsetX = x;
}

void Context::shadowOffsetY(float y)
{
	getCurrentState().shadowOffsetY = y;
}

void Context::shadowBlur(float blur)
{
	getCurrentState().shadowBlur = blur;
}

void Context::shadowColor(Ogre::ColourValue color)
{
	getCurrentState().shadowColor = color;
}

Gradient* Context::createLinearGradient(float x0, float y0, float x1, float y1)
{
	return new Gradient(x0, y0, x1, y1);
}

Gradient* Context::createLinearGradient(const Ogre::Vector2& pos0, const Ogre::Vector2& pos1)
{
	return createLinearGradient(pos0.x, pos0.y, pos1.x, pos1.y);
}

Gradient* Context::createRadialGradient(float x0, float y0, float radius0, float x1, float y1, float radius1)
{
	return new Gradient(x0, y0, radius0, x1, y1, radius1);
}

Gradient* Context::createRadialGradient(const Ogre::Vector2& pos0, float radius0, const Ogre::Vector2& pos1, float radius1)
{
	return createRadialGradient(pos0.x, pos0.y, radius0, pos1.x, pos1.y, radius1);
}

Pattern* Context::createPattern(const Ogre::Image& image, Repetition repeat)
{
	Pattern* pattern = new Pattern(image, repeat);
	return pattern;
}

//Paths
void Context::beginPath()
{
	mPath.reset();
}

void Context::closePath()
{
	mPath.close();
}

void Context::fill()
{	
	applyFillStyle();	
	mCanvas->drawPath(mPath, *mFillStyle);
}

void Context::stroke()
{
	applyStrokeStyle();
	mCanvas->drawPath(mPath, *mStrokeStyle);
	beginPath(); //is it needed ?
}

void Context::clip()
{
	mCanvas->clipPath(mPath);
}

void Context::moveTo(float x, float y)
{
	mPath.moveTo(x, y);
}

void Context::moveTo(const Ogre::Vector2& pos)
{
	moveTo(pos.x, pos.y);
}

void Context::lineTo(float x, float y)
{
	if (!pathHasCurrentPoint(mPath))
		mPath.moveTo(x, y);
	else
		mPath.lineTo(x, y);
}

void Context::lineTo(const Ogre::Vector2& pos)
{
	lineTo(pos.x, pos.y);
}

void Context::quadraticCurveTo(float cpx, float cpy,  float x, float y)
{
	if (!pathHasCurrentPoint(mPath))
		mPath.moveTo(x, y);
	else
		mPath.quadTo(cpx, cpy, x, y);
}

void Context:: quadraticCurveTo(const Ogre::Vector2& controlPoint, const Ogre::Vector2& pos)
{
	quadraticCurveTo(controlPoint.x, controlPoint.y, pos.x, pos.y);
}

void Context::bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y)
{
	if (!pathHasCurrentPoint(mPath))
		mPath.moveTo(x, y);
	else	
		mPath.cubicTo(cp1x, cp1y, cp2x, cp2y, x, y);
}

void Context:: bezierCurveTo(const Ogre::Vector2& controlPoint1, const Ogre::Vector2& controlPoint2, const Ogre::Vector2& pos)
{
	bezierCurveTo(controlPoint1.x, controlPoint1.y, controlPoint2.x, controlPoint2.y, pos.x, pos.y);
}

void Context:: arcTo(float x1, float y1, float x2, float y2, float radius)
{
	mPath.arcTo(x1, y1, x2, y2, radius);
}

void Context:: arcTo(const Ogre::Vector2& pos1, const Ogre::Vector2& pos2, float radius)
{
	arcTo(pos1.x, pos1.y, pos2.x, pos2.y, radius);
}

void Context::arc(float x, float y, float r, float startRadian, float endRadian, bool anticlockwise)
{
    SkScalar cx = x;
    SkScalar cy = y;
    SkScalar radius = r;
	SkScalar sa = startRadian;
	SkScalar ea = endRadian;	

    SkRect oval;
    oval.set(cx - radius, cy - radius, cx + radius, cy + radius);

    float sweep = ea - sa;
    // check for a circle
	if (sweep >= 2 * Ogre::Math::PI || sweep <= -2 * Ogre::Math::PI)
        mPath.addOval(oval);
    else {
        SkScalar startDegrees = (sa * 180 / Ogre::Math::PI);
        SkScalar sweepDegrees = (sweep * 180 / Ogre::Math::PI);

        // Counterclockwise arcs should be drawn with negative sweeps, while
        // clockwise arcs should be drawn with positive sweeps. Check to see
        // if the situation is reversed and correct it by adding or subtracting
        // a full circle
        if (anticlockwise && sweepDegrees > 0) {
            sweepDegrees -= SkIntToScalar(360);
        } else if (!anticlockwise && sweepDegrees < 0) {
            sweepDegrees += SkIntToScalar(360);
        }

        mPath.arcTo(oval, startDegrees, sweepDegrees, false);
    }
}

void Context::arc(const Ogre::Vector2& pos, float radius, Ogre::Radian start, Ogre::Radian end, bool anticlockwise)
{
	arc(pos.x, pos.y, radius, start.valueRadians(), end.valueRadians(), anticlockwise);
}

void Context::rect(float x, float y, float w, float h)
{
	mPath.addRect(x, y, w+x, h+y);
}

void Context::rect(const Ogre::Vector2& pos, const Ogre::Vector2& dim)
{
	rect(pos.x, pos.y, dim.x, dim.y);
}

bool Context::isPointInPath(float x, float y)
{
	return false;
}

bool Context::isPointInPath(const Ogre::Vector2& pos)
{
	return isPointInPath(pos.x, pos.y);
}

//Rectangles
void Context::clearRect(float x,  float y, float w, float h)
{
	save();
	globalCompositeOperation(Canvas::DrawingOperator_Clear);
	fillRect(x, y, w, h);
	restore();
}

void Context::clearRect(const Ogre::Vector2& pos,  const Ogre::Vector2& dim)
{
	clearRect(pos.x, pos.y, dim.x, dim.y);
}

void Context::fillRect(float x, float y, float w, float h)
{
	rect(x, y, w, h);
	fill();
	beginPath();
}

void Context::fillRect(const Ogre::Vector2& pos, const Ogre::Vector2& dim)
{
	fillRect(pos.x, pos.y, dim.x, dim.y);
}

void Context::strokeRect(float x, float y, float w, float h)
{
	rect(x, y, w, h);
	stroke();
	beginPath();
}

void Context::strokeRect(const Ogre::Vector2& pos, const Ogre::Vector2& dim)
{
	strokeRect(pos.x, pos.y, dim.x, dim.y);
}

//New
unsigned char* Context::getData()
{
	//SkAutoLockPixels image_lock(*mBitmap);
	return (unsigned char*) mBitmap->getPixels();
}

void Context::saveToFile(const std::string& filename)
{
	Ogre::Image image;
	image.loadDynamicImage(getData(), mWidth, mHeight, Ogre::PF_A8R8G8B8);
	image.save(filename);
}

void Context::applyFillStyle()
{
	State state = getCurrentState();
	mFillStyle->setXfermodeMode((SkXfermode::Mode)state.globalCompositeOperation);
	if (state.fillColourSource == ColourSource_Uniform)
	{
		Ogre::ColourValue color = state.fillColour;
		color.a *= state.globalAlpha;
		mFillStyle->setARGB((U8CPU)(color.a*255), (U8CPU)(color.r*255), (U8CPU)(color.g*255), (U8CPU)(color.b*255));
		mFillStyle->setShader(NULL);
		mFillStyle->setTextSize(state.textSize);
	}
	else if (state.fillColourSource == ColourSource_Gradient)
		mFillStyle->setShader(state.fillGradient->getShader());
	else //if (state.fillColourSource == ColourSource_Pattern)
		mFillStyle->setShader(state.fillPattern->getShader());
}

void Context::applyStrokeStyle()
{
	State state = getCurrentState();
	if (state.strokeColourSource == ColourSource_Uniform)
	{
		Ogre::ColourValue color = state.strokeColour;
		color.a *= state.globalAlpha;
		mStrokeStyle->setARGB((U8CPU)(color.a*255), (U8CPU)(color.r*255), (U8CPU)(color.g*255), (U8CPU)(color.b*255));
		mStrokeStyle->setStrokeMiter(state.miterLimit);
		mStrokeStyle->setStrokeJoin((SkPaint::Join) state.lineJoin);
		mStrokeStyle->setStrokeCap((SkPaint::Cap) state.lineCap);
		mStrokeStyle->setStrokeWidth(state.lineWidth);
		mStrokeStyle->setTextSize(state.textSize);
	}
	else if (state.strokeColourSource == ColourSource_Gradient)
		mStrokeStyle->setShader(state.strokeGradient->getShader());
	else //if (state.strokeColourSource == ColourSource_Pattern)
		mStrokeStyle->setShader(state.strokePattern->getShader());
}

bool Context::pathHasCurrentPoint(SkPath& path)
{
	return path.getPoints(NULL, 0) != 0;
}

State& Context::getCurrentState()
{
	return mStates.top();
}

Ogre::ColourValue ColourConverter::fromHexa(const std::string& text)
{
	std::string c = text;
	if (c.substr(0,1) == "#")
		c = c.substr(1, c.length() - 1);

	int r,g,b;
	
	std::stringstream ss;
			
	if (c.length() == 3) 
	{
		ss << std::setbase(16) << "0x" << c[0] << c[0];
		ss >> r;
		ss.clear();
		ss << std::setbase(16) << "0x" << c[1] << c[1];
		ss >> g;
		ss.clear();
		ss << std::setbase(16) << "0x" << c[2] << c[2];
		ss >> b;
		ss.clear();
	}
	else if (c.length() == 6) 
	{
		ss << std::setbase(16) << "0x" << c[0] << c[1];
		ss >> r;
		ss.clear();
		ss << std::setbase(16) << "0x" << c[2] << c[3];
		ss >> g;
		ss.clear();
		ss << std::setbase(16) << "0x" << c[4] << c[5];
		ss >> b;
		ss.clear();
	} 
	else
		return Ogre::ColourValue::Black;

	return Ogre::ColourValue(r / 255.0f, g / 255.0f, b / 255.0f, 1);
}

Ogre::ColourValue ColourConverter::fromRGBA(unsigned int r, unsigned int g, unsigned int b, unsigned int a)
{
	return Ogre::ColourValue(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

Gradient::Gradient(float x0, float y0, float x1, float y1)
{	
	mP0 = Ogre::Vector2(x0, y0);
	mP1 = Ogre::Vector2(x1, y1);
	mType = GradientType_Linear;
	mShader = NULL;
}

Gradient::Gradient(float x0, float y0, float radius0, float x1, float y1, float radius1)
{
	mP0 = Ogre::Vector2(x0, y0);
	mP1 = Ogre::Vector2(x1, y1);
	mRadius0 = radius0;
	mRadius1 = radius1;
	if (mP0 == mP1)
		mType = GradientType_Radial;
	else
		mType = GradientType_RadialWithFocalPoint;
	mShader = NULL;
}

Gradient::~Gradient()
{
	deleteShader();
}

void Gradient::addColorStop(float offset, Ogre::ColourValue color)
{	
	mColorStops.push_back(ColorStop(offset, color));
	deleteShader();
}

SkShader* Gradient::getShader()
{
	if (mShader == NULL)
		createShader();

	return mShader;
}
void Gradient::createShader()
{
	//FIXME : how is std::sort behaving with two identical values ?
	//std::sort(mColorStops.begin(), mColorStops.end(), colorStopsSorting);

	SkPoint pts[2];
	pts[0].set(mP0.x, mP0.y);
	pts[1].set(mP1.x, mP1.y);

	SkColor* colors = new SkColor[mColorStops.size()];
	SkScalar* pos   = new SkScalar[mColorStops.size()];
	for (unsigned int i=0; i<mColorStops.size(); ++i)
	{
		Ogre::ColourValue color = mColorStops[i].second;
		colors[i] = SkColorSetARGB((U8CPU)(color.a*255), (U8CPU)(color.r*255), (U8CPU)(color.g*255), (U8CPU)(color.b*255));
		pos[i]    = mColorStops[i].first;
	}

	if (mType == GradientType_Linear)
		mShader = SkGradientShader::CreateLinear(pts, colors, pos, mColorStops.size(), SkShader::kClamp_TileMode);
	else if (mType == GradientType_Radial)
		mShader = SkGradientShader::CreateRadial(pts[1], mRadius1, colors, pos, mColorStops.size(), SkShader::kClamp_TileMode);
	else
		mShader = SkGradientShader::CreateTwoPointRadial(pts[0], mRadius0, pts[1], mRadius1, colors, pos, mColorStops.size(), SkShader::kClamp_TileMode);
}

void Gradient::deleteShader()
{
	SkSafeUnref(mShader);
	mShader = NULL;
}

bool Gradient::colorStopsSorting(ColorStop a, ColorStop b)
{ 
	return a.first < b.first;
}

Pattern::Pattern(const Ogre::Image& img, Repetition repeat)
{	
	SkBitmap bitmap;// = new SkBitmap();
	bitmap.setConfig(SkBitmap::kARGB_8888_Config, img.getWidth(), img.getHeight());
	bitmap.allocPixels();
	bitmap.eraseARGB(0x00, 0x00, 0x00, 0x00);	

	Ogre::PixelBox src = img.getPixelBox();
	Ogre::PixelBox dst = Ogre::PixelBox(src.getWidth(), src.getHeight(), src.getDepth(), Ogre::PF_A8R8G8B8); //PF_A8B8G8R8

	dst.data = bitmap.getPixels();
	Ogre::PixelUtil::bulkPixelConversion(src, dst);

	if (repeat == Repetition_Repeat)
		mShader = SkShader::CreateBitmapShader(bitmap, SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode);
	else
	{
		bool repeatX = repeat == Repetition_RepeatX;
		bool repeatY = repeat == Repetition_RepeatY;
		SkShader::TileMode tileModeX = repeatX ? SkShader::kRepeat_TileMode : SkShader::kClamp_TileMode;
		SkShader::TileMode tileModeY = repeatY ? SkShader::kRepeat_TileMode : SkShader::kClamp_TileMode;
		int expandW = repeatX ? 0 : 1;
		int expandH = repeatY ? 0 : 1;	

		SkBitmap bitmapTmp;
		bitmapTmp.setConfig(bitmap.config(), bitmap.width() + expandW, bitmap.height() + expandH);
		bitmapTmp.allocPixels();
		bitmapTmp.eraseARGB(0x00, 0x00, 0x00, 0x00);
		SkCanvas canvas(bitmapTmp);
		canvas.drawBitmap(bitmap, 0, 0);
		mShader = SkShader::CreateBitmapShader(bitmapTmp, tileModeX, tileModeY);
	}
}

Pattern::~Pattern()
{
	SkSafeUnref(mShader);
	mShader = NULL;
}

TextMetrics::TextMetrics(float xBearing, float yBearing, float width, float height, float xAdvance, float yAdvance)
{
	this->xBearing = xBearing;
	this->yBearing = yBearing;
	this->width    = width;
	this->height   = height;
	this->xAdvance = xAdvance;
	this->yAdvance = yAdvance;
}

float TextMetrics::getFullWidth() 
{  
	return width + xBearing; 
}

float TextMetrics::getFullHeight() 
{ 
	return height; // + yBearing removed yBearing because for most fonts it returns a minus.}
}

void Context::fillText(const std::string& text, float x, float y, float maxWidth)
{
	applyFillStyle();
	mCanvas->drawText(text.c_str(), text.size(), x, y, *mFillStyle);
}

void Context::fillText(const std::string& text, const Ogre::Vector2& pos, float maxWidth)
{
	fillText(text, pos.x, pos.y, maxWidth);
}

void Context::strokeText(const std::string& text, float x, float y, float maxWidth)
{
	applyStrokeStyle();
	mCanvas->drawText(text.c_str(), text.size(), x, y, *mStrokeStyle);
}

void Context::strokeText(const std::string& text, const Ogre::Vector2& pos, float maxWidth)
{
	strokeText(text, pos.x, pos.y, maxWidth);
}