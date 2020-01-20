#include "App.h"
#include "Settings.h"


using namespace ckeys;


//  Key pressed
//-----------------------------------------------
void App::KeyDown(const ImGuiKey key)
{
	//  close options window
	if (options || graphics)
	{
		switch (key)
		{
		case ImGuiKey_Escape:
		case ImGuiKey_Enter:
			options = graphics = false;
		default:;
		}
	}
}

//  Mouse move
//-----------------------------------------------
void App::Mouse(int x, int y)
{
	xm = x;  ym = y;
}
