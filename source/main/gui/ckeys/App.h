#pragma once
#include "AppDraw.h"
#include "Settings.h"
#include "Keys.h"
#include "OgreImGui.h"

namespace ckeys {

class App : public AppDraw
{
public:
	//  main
	App();
	~App();

	bool Init();
	void Resize(int x, int y);

	void KeyDown(const ImGuiKey key);
	void Mouse(int x, int y);

	void Graph();  // draw graphics, keys
	void Gui();   // draw Gui


	//  gui util  -----
	void SetupGuiClr();
	void Sep(int y);  // dummy separator
	void Line(bool dark = false);  //--

	Settings set;

	Keys keys;


	//  gui windows
	bool options = false;
	bool graphics = false;
	bool help = false;

	//  auto max size
	int xMax = 0, yMax = 0;

	//  mouse pos
	int xm = 0, ym = 0;
};

} // namespace ckeys
