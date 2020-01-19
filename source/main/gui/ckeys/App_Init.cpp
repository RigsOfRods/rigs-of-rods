#include "App.h"
#include "Keys.h"


//  ctor
App::App()
{

}

//  Init
bool App::Init()
{
	SetupGuiClr();

	keys.Init(&set);

	keys.LoadIndex();

	keys.Hook();

	return true;
}

//  dtor
App::~App()
{
	keys.UnHook();
}


void App::Resize(int x, int y)
{
	set.xwSize = x;  set.ywSize = y;
	if (pWindow)
		pWindow->setView(sf::View(sf::FloatRect(0.f, 0.f, set.xwSize, set.ywSize)));
}
