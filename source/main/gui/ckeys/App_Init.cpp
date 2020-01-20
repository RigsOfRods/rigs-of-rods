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




