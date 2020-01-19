#pragma once
#include "OgreImGui.h"

#include <string>

namespace ckeys {

class AppDraw
{
public:

	int text_character_size = 10;

	std::string str;
	ImColor clr;
	bool bold = false;

	float dt = 1.f;  // frame delta time
	float fps = 1.f;  // fps averaged


	//  set text color
	//--------------------------
	void Clr(uint8_t r, uint8_t g, uint8_t b)
	{
		clr = ImColor(r,g,b, 1);
	}

	//  write out text, from s
	//  returns width, x advance
	int Txt(int x, int y, bool draw=true);

	//  fill rect
	void Rect(int x, int y,  int sx, int sy,  uint8_t r, uint8_t g, uint8_t b);
	//  frame rect
	void Frame(int x, int y,  int sx, int sy,  int d,  uint8_t r, uint8_t g, uint8_t b);
};

} // namespace ckeys
