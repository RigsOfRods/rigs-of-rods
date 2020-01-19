#include "AppMain.h"
#include <locale.h>
#include <cstdlib>


//  Main
///-----------------------------------------------------------------------------
int main()
{
	setlocale(LC_NUMERIC, "C");

	//  Start
	//-----------------------------------

	AppMain* pApp = new AppMain();

	pApp->Run();


	delete pApp;

	return EXIT_SUCCESS;
}
