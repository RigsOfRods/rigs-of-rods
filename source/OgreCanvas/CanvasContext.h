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

#pragma once

#include <Ogre.h> //must be include before skia otherwise you get error with std::min & std::max (NOMINMAX definition in skia ?)
#include "CanvasPrerequisites.h"

#ifndef SK_IGNORE_STDINT_DOT_H
#define SK_IGNORE_STDINT_DOT_H
#endif

#include <core/SkBitmap.h>
#include <core/SkDevice.h>
#include <core/SkPaint.h>
#include <core/SkRect.h>
#include <core/SkPath.h>

#include <stack>



namespace Ogre
{
	namespace Canvas
	{
		enum DrawingOperator 
		{
			DrawingOperator_Copy        = SkXfermode::kSrcOver_Mode, //dont't known which one it is
			DrawingOperator_SourceOver  = SkXfermode::kSrcOver_Mode,
			DrawingOperator_SourceIn    = SkXfermode::kSrcIn_Mode,
			DrawingOperator_SourceOut   = SkXfermode::kSrc_Mode,
			DrawingOperator_SourceATop  = SkXfermode::kSrcATop_Mode,
			DrawingOperator_DestOver    = SkXfermode::kDstOver_Mode,
			DrawingOperator_DestIn      = SkXfermode::kDstIn_Mode,
			DrawingOperator_DestOut     = SkXfermode::kDst_Mode,
			DrawingOperator_DestATop    = SkXfermode::kDstATop_Mode,
			DrawingOperator_Xor         = SkXfermode::kXor_Mode,
			DrawingOperator_PlusDarker  = SkXfermode::kDarken_Mode,  //dont't known which one it is
			DrawingOperator_Highlight   = SkXfermode::kSrcOver_Mode, //dont't known which one it is
			DrawingOperator_PlusLighter = SkXfermode::kLighten_Mode, //dont't known which one it is
			DrawingOperator_Clear       = SkXfermode::kClear_Mode
		};

		enum LineCap
		{
			LineCap_Butt   = SkPaint::kButt_Cap,
			LineCap_Round  = SkPaint::kRound_Cap,
			LineCap_Square = SkPaint::kSquare_Cap
		};

		enum LineJoin
		{
			LineJoin_Round = SkPaint::kRound_Join,
			LineJoin_Bevel = SkPaint::kBevel_Join,
			LineJoin_Miter = SkPaint::kMiter_Join
		};

		enum LineDash
		{
			LineDash_Solid,
			LineDash_Dotted,
			LineDash_Dashed
		};

		enum Repetition
		{
			Repetition_Repeat,
			Repetition_RepeatX,
			Repetition_RepeatY,
			Repetition_NoRepeat
		};

		//JS values : Start, End, Left, Right, Center
		enum TextAlign
		{
			TextAlign_Left   = SkPaint::kLeft_Align,
			TextAlign_Center = SkPaint::kCenter_Align,
			TextAlign_Right  = SkPaint::kRight_Align
		};

		enum TextBaseline
		{
			TextBaseline_Top,
			TextBaseline_Hanging,
			TextBaseline_Middle,
			TextBaseline_Alphabetic,
			TextBaseline_Ideographic, 
			TextBaseline_Bottom
		};

		enum ColourSource
		{
			ColourSource_Uniform,
			ColourSource_Gradient,
			ColourSource_Pattern
		};

		enum GradientType
		{
			GradientType_Linear,
			GradientType_Radial,
			GradientType_RadialWithFocalPoint
		};

		class _OgreCanvasExport TextMetrics
		{
			public:
				TextMetrics(float xBearing, float yBearing, float width, float height, float xAdvance, float yAdvance);
				float getFullWidth();
				float getFullHeight();

				Ogre::Real xBearing;
				Ogre::Real yBearing;
				Ogre::Real width;
				Ogre::Real height;
				Ogre::Real xAdvance;
				Ogre::Real yAdvance;
		};

		class _OgreCanvasExport Colour //unused
		{
			public:
				Colour(const Ogre::ColourValue& color);
				Colour(unsigned int r, unsigned int g, unsigned int b, Ogre::Real a = 1.0);
				Colour(const std::string& hexa);

				Ogre::Real r;
				Ogre::Real g;
				Ogre::Real b;
				Ogre::Real a;
		};

		typedef std::pair<Ogre::Real, Ogre::ColourValue> ColorStop;

		class _OgreCanvasExport Gradient
		{
			
			public:
				Gradient(float x0, float y0, float x1, float y1);
				Gradient(float x0, float y0, float radius0, float x1, float y1, float radius1);
				~Gradient();

				void addColorStop(float offset, Ogre::ColourValue color);
				friend class Context;

			protected:
				SkShader* getShader();
				void createShader();
				void deleteShader();

				static bool colorStopsSorting(ColorStop a, ColorStop b);

				SkShader* mShader;
				GradientType  mType;
				Ogre::Vector2 mP0;
				Ogre::Vector2 mP1;
				Ogre::Real    mRadius0;
				Ogre::Real    mRadius1;
				std::vector<ColorStop> mColorStops;

		};

		class _OgreCanvasExport Pattern
		{
			public:
				Pattern(const Ogre::Image& img, Repetition repeat);
				~Pattern();

				friend class Context;

			protected:
				SkShader* getShader() { return mShader; }
				SkShader* mShader;
		};

		class _OgreCanvasExport ImageData
		{
			public:
				ImageData();

				int width();
				int height();
				unsigned char* data();

			protected:
				unsigned char* mData;
		};

		class _OgreCanvasExport State 
		{
			public:
				State();
				
				//Compositing
				Ogre::Real      globalAlpha;
				DrawingOperator globalCompositeOperation;
				
				//Line styles
				Ogre::Real  lineWidth;
				LineCap     lineCap;
				LineJoin    lineJoin;
				Ogre::Real  miterLimit;
				LineDash    lineDash;
				
				//Stroke and fill style
				ColourSource      strokeColourSource;			
				Ogre::ColourValue strokeColour;
				Gradient*         strokeGradient;
				Pattern*          strokePattern;

				ColourSource      fillColourSource;
				Ogre::ColourValue fillColour;
				Gradient*         fillGradient;
				Pattern*          fillPattern;
				
				//Shadow
				Ogre::Real        shadowOffsetX;
				Ogre::Real        shadowOffsetY;
				Ogre::Real        shadowBlur;
				Ogre::ColourValue shadowColor;

				//TransformationMatrix m_transform;
				//bool m_invertibleCTM;

				// Text
				Ogre::Real   textSize;
				TextAlign    textAlign;
				TextBaseline textBaseline;

				//String m_unparsedFont;
				//Font m_font;
				//bool m_realizedFont;
		};

		class _OgreCanvasExport Context
		{
			public:
				Context(unsigned int _width, unsigned int _height, bool _enableAlpha = true);
				~Context();

				//2D Context			
				void save();
				void restore();
				void reset();

				//Transformation					
				void scale(float x, float y);
				void scale(const Ogre::Vector2& scaling);			

				void rotate(float radian);
				void rotate(const Ogre::Radian& angle);
				
				void translate(float x, float y);
				void translate(const Ogre::Vector2& translation);

				void transform(float m11, float m12, float m21, float m22, float dx,  float dy);
				void transform(const Ogre::Matrix3& transform);

				void setTransform(float m11, float m12, float m21, float m22, float dx,  float dy);
				void setTransform(const Ogre::Matrix3& transform);

				//Image drawing			
				void drawImage(const Ogre::Image& image, float dstX, float dstY, float dstWidth = 0, float dstHeight = 0);
				void drawImage(const Ogre::Image& image, const Ogre::Vector2& pos, const Ogre::Vector2& dim = Ogre::Vector2::ZERO);
				
				void drawImage(const Ogre::Image& image, float srcX, float srcY, float srcWidth, float srcHeight, float dstX, float dstY, float dstWidth, float dstHeight);
				void drawImage(const Ogre::Image& image, const Ogre::Vector2& srcPos, const Ogre::Vector2& srcDim, const Ogre::Vector2& dstPos, const Ogre::Vector2& dstDim);

				//Compositing
				void globalAlpha(float alpha);
				float globalAlpha();

				void globalCompositeOperation(DrawingOperator op);
				DrawingOperator globalCompositeOperation();

				//Line styles
				void lineWidth(float width);
				float lineWidth();

				void lineCap(LineCap lineCap);
				LineCap lineCap();

				void lineJoin(LineJoin lineJoin);
				LineJoin lineJoin();

				void miterLimit(float limit);
				float miterLimit();

				void lineDash(LineDash lineDash);
				LineDash lineDash();

				//Colors, styles and shadows
				void strokeStyle(Ogre::ColourValue color);
				void strokeStyle(Gradient* gradient);
				void strokeStyle(Pattern* pattern);

				void fillStyle(Ogre::ColourValue color);
				void fillStyle(Gradient* gradient);
				void fillStyle(Pattern* pattern);

				void shadowOffsetX(float x);
				void shadowOffsetY(float y);
				void shadowBlur(float blur);
				void shadowColor(Ogre::ColourValue color);
				
				Gradient* createLinearGradient(float x0, float y0, float x1, float y1);
				Gradient* createLinearGradient(const Ogre::Vector2& pos0, const Ogre::Vector2& pos1);
				
				Gradient* createRadialGradient(float x0, float y0, float radius0, float x1, float y1, float radius1);
				Gradient* createRadialGradient(const Ogre::Vector2& pos0, float radius0, const Ogre::Vector2& pos1, float radius1);

				Pattern* createPattern(const Ogre::Image& image, Repetition = Repetition_Repeat);
				
				//Paths
				void beginPath();
				void closePath();
				void fill();
				void stroke();
				void clip();
				
				void moveTo(float x, float y);
				void moveTo(const Ogre::Vector2& pos);
				
				void lineTo(float x, float y);
				void lineTo(const Ogre::Vector2& pos);
				
				void quadraticCurveTo(float cpx, float cpy, float x, float y);
				void quadraticCurveTo(const Ogre::Vector2& controlPoint, const Ogre::Vector2& pos);
				
				void bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y);
				void bezierCurveTo(const Ogre::Vector2& controlPoint1, const Ogre::Vector2& controlPoint2, const Ogre::Vector2& pos);
				
				void arcTo(float x1, float y1, float x2, float y2, float radius);
				void arcTo(const Ogre::Vector2& pos1, const Ogre::Vector2& pos2, float radius);
				
				void arc(float x, float y, float radius, float startRadian, float endRadian, bool anticlockwise);
				void arc(const Ogre::Vector2& pos, float radius, Ogre::Radian start, Ogre::Radian end, bool anticlockwise);
				
				void rect(float x, float y, float w, float h);
				void rect(const Ogre::Vector2& pos, const Ogre::Vector2& dim);
				
				bool isPointInPath(float x, float y);
				bool isPointInPath(const Ogre::Vector2& pos);	

				//Text
				void textSize(float size);
				float textSize();

				void textFont(const std::string& family);
				std::string textFont();

				void textAlign(TextAlign align);
				TextAlign textAlign();

				void textBaseline(TextBaseline baseline);
				TextBaseline textBaseline();
				
				void fillText(const std::string& text, Ogre::Real x, Ogre::Real y, Ogre::Real maxWidth = -1);
				void fillText(const std::string& text, const Ogre::Vector2& pos, Ogre::Real maxWidth = -1);
				
				void strokeText(const std::string& text, Ogre::Real x, Ogre::Real y, Ogre::Real maxWidth = -1);
				void strokeText(const std::string& text, const Ogre::Vector2& pos, Ogre::Real maxWidth = -1);			

				TextMetrics measureText(const std::string& text);

				//Rectangles			
				void clearRect(Ogre::Real x,  Ogre::Real y, Ogre::Real w, Ogre::Real h);
				void clearRect(const Ogre::Vector2& pos, const Ogre::Vector2& dim);
				
				void fillRect(Ogre::Real x, Ogre::Real y, Ogre::Real w, Ogre::Real h);
				void fillRect(const Ogre::Vector2& pos, const Ogre::Vector2& dim);
				
				void strokeRect(Ogre::Real x, Ogre::Real y, Ogre::Real w, Ogre::Real h);
				void strokeRect(const Ogre::Vector2& pos, const Ogre::Vector2& dim);
				
				ImageData createImageData(Ogre::Real width, Ogre::Real height);
				ImageData createImageData(const Ogre::Vector2& dim);

				ImageData createImageData(ImageData imageData);
				
				ImageData getImageData(Ogre::Real x, Ogre::Real y, Ogre::Real width, Ogre::Real height);
				ImageData getImageData(const Ogre::Vector2& pos, const Ogre::Vector2& dim);
				
				void putImageData(ImageData imageData, Ogre::Real x, Ogre::Real y, Ogre::Real dirtyX = 0, Ogre::Real dirtyY = 0);
				void putImageData(ImageData imageData, const Ogre::Vector2& pos, const Ogre::Vector2& dirty = Ogre::Vector2::ZERO);
				
				//New
				unsigned char* getData();
				void saveToFile(const std::string& _filename);
				unsigned int width()  { return mWidth;  }
				unsigned int height() { return mHeight; }

			protected:						
				void applyFillStyle();
				void applyStrokeStyle();
				bool pathHasCurrentPoint(SkPath& path);

				State& getCurrentState();

				unsigned int      mWidth;
				unsigned int      mHeight;
				bool              mAlphaEnable;
				SkPath            mPath;
				SkBitmap*         mBitmap;
				SkDevice*         mDevice;
				SkCanvas*         mCanvas;
				SkPaint*          mFillStyle;
				SkPaint*          mStrokeStyle;

				std::stack<State> mStates;
		};

		class _OgreCanvasExport ColourConverter
		{
			public:
				static Ogre::ColourValue fromHexa(const std::string& text);
				static Ogre::ColourValue fromRGBA(unsigned int r, unsigned int g, unsigned int b, unsigned int a = 255);
		};
	}
}