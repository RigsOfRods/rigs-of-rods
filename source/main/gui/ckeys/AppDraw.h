#pragma once


namespace ckeys {

class AppDraw
{
public:

//	//  sfml draw
//	//--------------------------
//	sf::RenderWindow* pWindow = nullptr;
//	sf::Sprite* pBackgr = nullptr;
//
//	sf::Font* pFont = nullptr;
//	sf::Text text;
//
//	std::string str;
//	sf::Color clr;
	bool bold = false;

	float dt = 1.f;  // frame delta time
	float fps = 1.f;  // fps averaged


	//  set text color
	//--------------------------
	void Clr(sf::Uint8 r, sf::Uint8 g, sf::Uint8 b)
	{
		clr = sf::Color(r,g,b);
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
