#include "App.h"
#include "Keys.h"

using namespace ckeys;

//  ctor
App::App()
{

}

//  Init
bool App::Init()
{
//	SetupGuiClr(); // Rigs of Rods: Done per frame

	keys.Init(&set);

	keys.LoadIndex();

	return true;
}

//  dtor
App::~App()
{

}


