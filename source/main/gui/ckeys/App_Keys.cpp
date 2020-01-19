#include "App.h"
#include "Settings.h"

using namespace ckeys;

#define ret		 return true;


//  Key pressed
//-----------------------------------------------
bool App::KeyDown(const sf::Event::KeyEvent& key)
{
	using namespace sf;

	//  close options window
	if (options || graphics)
	{
		switch (key.code)
		{
		case Keyboard::Escape:
		case Keyboard::Return:
		case Keyboard::Space:
			options = graphics = false;
		}
		ret
	}

	switch (key.code)
	{
		//  Esc - Close
		case Keyboard::Escape:
			if (set.escQuit)  pWindow->close();
			ret
	}
	ret
}

//  Mouse move
//-----------------------------------------------
void App::Mouse(int x, int y)
{
	xm = x;  ym = y;
}
